#include "Ball.h"
#include "Sphere.h"
#include "BindableCommon.h"
#include "AttackShadeCbuf.h"
#include "Channels.h"

Ball_Shape::Ball_Shape(Graphics& gfx, DirectX::XMFLOAT3 size)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model_Head = Sphere::Make();
	model_Head.Transform(dx::XMMatrixScaling(size.x, size.y, size.z));
	const auto geometryTag = "$sphere." + std::to_string(size.x);

	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model_Head.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model_Head.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique head{ "Shade", Chan::main };
		Step only("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Attack_VS.cso");
		only.AddBindable(InputLayout::Resolve(gfx, model_Head.vertices.GetLayout(), *pvs));
		only.AddBindable(std::move(pvs));

		only.AddBindable(PixelShader::Resolve(gfx, "Attack_PS.cso"));

		// 像素时间。Attack_PS 用 totalTime 转动竖条，寄存器 b1。
		only.AddBindable(std::make_shared<TimeCbuf_PS>(gfx));

		// 留住这一发的颜色缓冲，和步骤里绑定的是同一个。默认编号 0，外层不透明。
		shade_ = std::make_shared<AttackShadeCbuf>(gfx);
		only.AddBindable(shade_);

		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));

		head.AddStep(std::move(only));
		AddTechnique(std::move(head));
	}

	{
		// 外层变淡时改走这条。默认关掉，避免没变淡的球进透明通道。
		Technique fade{ "ShadeFade", Chan::main, false };
		Step only("lambertianTrans");

		auto pvs = VertexShader::Resolve(gfx, "Attack_VS.cso");
		only.AddBindable(InputLayout::Resolve(gfx, model_Head.vertices.GetLayout(), *pvs));
		only.AddBindable(std::move(pvs));
		only.AddBindable(PixelShader::Resolve(gfx, "Attack_PS.cso"));
		only.AddBindable(std::make_shared<TimeCbuf_PS>(gfx));
		only.AddBindable(shade_);
		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));
		only.AddBindable(Blender::Resolve(gfx, true));

		fade.AddStep(std::move(only));
		AddTechnique(std::move(fade));
	}

}

void Ball_Shape::ClearColorIds() noexcept
{
	// 外形还没接上颜色缓冲时跳过。透明度一并收回不透明，并回到原来的绘制通道。
	if (shade_ != nullptr)
	{
		shade_->ClearColorIds();
	}
	SetOuterOpacity(1.0f);
}

void Ball_Shape::AppendColorId(std::uint32_t colorId)
{
	// colorId 是这一条的颜色编号。缓冲还没接上时跳过。
	if (shade_ != nullptr)
	{
		shade_->AppendColorId(colorId);
	}
}

void Ball_Shape::SetOuterOpacity(float opacity) noexcept
{
	if (shade_ != nullptr)
	{
		shade_->SetOpacity(opacity);
	}
	// fade 为真时这颗球改走透明通道，中心按视线变透。别的球仍用原来的不透明通道。
	const bool fade = opacity < 1.0f;
	for (Technique& tech : techniques)
	{
		if (tech.GetName() == "Shade")
		{
			tech.SetActiveState(!fade);
		}
		else if (tech.GetName() == "ShadeFade")
		{
			tech.SetActiveState(fade);
		}
	}
}

DirectX::XMMATRIX Ball_Shape::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}