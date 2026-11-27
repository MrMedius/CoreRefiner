#include "Sprite2DPolygon.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "SpriteUVCbuf.h"
#include "DynamicTexture.h"
#include "RingParamsCbuf.h"
#include "Channels.h"

Sprite2DPolygon::Sprite2DPolygon(Graphics& gfx, std::vector<std::string> paths, int sides)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = PlanePolygon::Make2D(sides);
	const auto geometryTag = "$planePoly." + paths[0] + std::to_string(paths[0].size());
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<Transform2DCbuf>(gfx);
	auto uvcb = std::make_shared<SpriteUVCbuf>(gfx);
	auto rpcb = std::make_shared<RingParamsCbuf>(gfx);
	auto dynTex = std::make_shared<DynamicTexture>(gfx, paths);
	// 留住下面要绑定的那一份帧贴图和 UV 缓冲。
	AttachFrameBinds(dynTex, uvcb);
	// 留住这一份环参数缓冲，和步骤里绑定的是同一个。
	rpcb_ = rpcb;

	{
		Technique ui("UI", Chan::ui);
		{
			Step draw("ui");

			draw.AddBindable(dynTex);
			draw.AddBindable(Sampler::Resolve(gfx));

			auto pvs = VertexShader::Resolve(gfx, "Sprite2DPolygon_VS.cso");
			draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			draw.AddBindable(std::move(pvs));

			draw.AddBindable(PixelShader::Resolve(gfx, "Sprite2DPolygon_PS.cso"));
			draw.AddBindable(rpcb);

			draw.AddBindable(tcb);
			draw.AddBindable(uvcb);

			ui.AddStep(std::move(draw));
		}
		AddTechnique(std::move(ui));
	}
	// 构造结束时把当前环参数写进缓冲。
	if (rpcb_ != nullptr)
	{
		rpcb_->SetRing(ringParams);
	}
}

// setters
void Sprite2DPolygon::SetRingRange(float start, float end) noexcept
{
	ringParams.startAngleDeg = start;
	ringParams.endAngleDeg = end;
	// start、end 是起止角。写进环参数缓冲。
	if (rpcb_ != nullptr)
	{
		rpcb_->SetRing(ringParams);
	}
}

void Sprite2DPolygon::SetRingRatio(float ratio) noexcept
{
	ringParams.ratio = ratio;
	// ratio 是环的进度。写进环参数缓冲。
	if (rpcb_ != nullptr)
	{
		rpcb_->SetRing(ringParams);
	}
}