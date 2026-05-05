#include "Canvas2D.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "SpriteUVCbuf.h"
#include "CanvasTexture.h"
#include "Channels.h"

Canvas2D::Canvas2D(Graphics& gfx, unsigned width, unsigned height)
	:
	Canvas(width, height)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Plane::Make2D(1, 1);
	const auto geometryTag = std::string("$canvas2d.") + std::to_string(width) + "x" + std::to_string(height);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<Transform2DCbuf>(gfx);
	auto uvcb = std::make_shared<SpriteUVCbuf>(gfx);
	auto canvasTex = std::make_shared<CanvasTexture>(gfx);

	Technique ui("UI", Chan::ui);
	{
		Step draw("ui");

		draw.AddBindable(canvasTex);
		draw.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));

		auto pvs = VertexShader::Resolve(gfx, "Sprite2D_VS.cso");
		draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		draw.AddBindable(std::move(pvs));

		draw.AddBindable(PixelShader::Resolve(gfx, "Sprite2D_PS.cso"));

		draw.AddBindable(tcb);
		draw.AddBindable(uvcb);

		ui.AddStep(std::move(draw));
	}
	AddTechnique(std::move(ui));
}