#include "Canvas2DSpriteUV.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "CanvasTexture.h"
#include "SpriteUVCbuf.h"
#include "Channels.h"

Canvas2DSpriteUV::Canvas2DSpriteUV(Graphics& gfx, unsigned width, unsigned height)
	:
	Canvas(width, height)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Plane::Make2D(1, 1);
	const auto geometryTag = std::string("$canvas2d.spriteuv.")
		+ std::to_string(width) + "x" + std::to_string(height);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<Transform2DCbuf>(gfx);
	auto uvcb = std::make_shared<SpriteUVCbuf>(gfx);
	auto canvasTex = std::make_shared<CanvasTexture>(gfx);
	// 留住这一份 UV 缓冲，和步骤里绑定的是同一个。
	uvcb_ = uvcb;

	Technique ui("UI", Chan::ui);
	{
		Step draw("ui");

		draw.AddBindable(canvasTex);
		draw.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));

		auto pvs = VertexShader::Resolve(gfx, "Canvas2DSpriteUV_VS.cso");
		draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		draw.AddBindable(std::move(pvs));

		draw.AddBindable(PixelShader::Resolve(gfx, "Canvas2DSpriteUV_PS.cso"));

		draw.AddBindable(tcb);
		draw.AddBindable(uvcb);

		ui.AddStep(std::move(draw));
	}
	AddTechnique(std::move(ui));
	// 构造结束时写入默认 UV。起点 0，裁切缩放 1，采样窗口 1。
	SyncUV_();
}

void Canvas2DSpriteUV::SetUVOffset(DirectX::XMFLOAT2 offset) noexcept
{
	// offset 是采样起点。
	uvOffset_ = offset;
	SyncUV_();
}

void Canvas2DSpriteUV::SetUVScale(DirectX::XMFLOAT2 scale) noexcept
{
	// scale 是裁切用的缩放。
	uvScale_ = scale;
	SyncUV_();
}

void Canvas2DSpriteUV::SetSampleScale(DirectX::XMFLOAT2 sampleScale) noexcept
{
	// sampleScale 是采样窗口。
	sampleScale_ = sampleScale;
	SyncUV_();
}

void Canvas2DSpriteUV::SyncUV_() noexcept
{
	if (uvcb_ == nullptr)
	{
		return;
	}
	Bind::SpriteUVCbuf::UVData uv{};
	uv.offset = uvOffset_;
	uv.scale = uvScale_;
	uv.sampleScale = sampleScale_;
	uvcb_->SetUV(uv);
}
