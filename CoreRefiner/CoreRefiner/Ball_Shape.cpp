#include "Ball_Shape.h"
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

		// 留住这一发的颜色缓冲，和步骤里绑定的是同一个。默认编号 0。
		shade_ = std::make_shared<AttackShadeCbuf>(gfx);
		only.AddBindable(shade_);

		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));

		head.AddStep(std::move(only));
		AddTechnique(std::move(head));
	}

}

void Ball_Shape::ClearColorIds() noexcept
{
	// 外形还没接上颜色缓冲时跳过。
	if (shade_ != nullptr)
	{
		shade_->ClearColorIds();
	}
}

void Ball_Shape::AppendColorId(std::uint32_t colorId)
{
	// colorId 是这一条的颜色编号。缓冲还没接上时跳过。
	if (shade_ != nullptr)
	{
		shade_->AppendColorId(colorId);
	}
}

DirectX::XMMATRIX Ball_Shape::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}