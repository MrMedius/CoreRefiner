#include "Sprite2D.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "SpriteUVCbuf.h"
#include "DynamicTexture.h"
#include "Channels.h"

Sprite2D::Sprite2D(Graphics& gfx, std::vector<std::string> paths)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Plane::Make2D(1, 1);
	const auto geometryTag = "$plane." + paths[0] + std::to_string(paths[0].size());
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<Transform2DCbuf>(gfx);
	auto uvcb = std::make_shared<SpriteUVCbuf>(gfx);
	auto dynTex = std::make_shared<DynamicTexture>(gfx, paths);

	{
		Technique ui("UI", Chan::ui);
		{
			Step draw("ui");

			draw.AddBindable(dynTex);
			draw.AddBindable(Sampler::Resolve(gfx));

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
}