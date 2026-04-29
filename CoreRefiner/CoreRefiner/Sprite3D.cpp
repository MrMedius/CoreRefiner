#include "Sprite3D.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include "SpriteUVCbuf.h"
#include "DynamicTexture.h"
#include "Channels.h"

Sprite3D::Sprite3D(Graphics& gfx, std::vector<std::string> paths)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Plane::Make();
	model.SetNormalsIndependentFlat();
	const auto geometryTag = "$plane." + paths[0] + std::to_string(paths[0].size());
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<TransformCbuf>(gfx);
	auto uvcb = std::make_shared<SpriteUVCbuf>(gfx);
	auto dynTex = std::make_shared<DynamicTexture>(gfx, paths);

	{
		Technique shade("Shade", Chan::main);
		{
			Step only("lambertian");

			only.AddBindable(dynTex);
			only.AddBindable(Sampler::Resolve(gfx));

			auto pvs = VertexShader::Resolve(gfx, "Sprite3D_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));

			only.AddBindable(PixelShader::Resolve(gfx, "Sprite3D_PS.cso"));

			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("specularColor");
			lay.Add<Dcb::Float>("specularWeight");
			lay.Add<Dcb::Float>("specularGloss");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["specularColor"] = dx::XMFLOAT3{ 1.0f,1.0f,1.0f };
			buf["specularWeight"] = 0.1f;
			buf["specularGloss"] = 20.0f;
			only.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

			only.AddBindable(Rasterizer::Resolve(gfx, true));

			only.AddBindable(tcb);
			only.AddBindable(uvcb);

			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
	}
	{
		Technique outline("Outline", Chan::main);
		{
			Step mask("outlineMask");

			auto pvs = VertexShader::Resolve(gfx, "Sprite3D_Mask_VS.cso");
			mask.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			mask.AddBindable(std::move(pvs));

			mask.AddBindable(dynTex);
			mask.AddBindable(Sampler::Resolve(gfx));

			mask.AddBindable(PixelShader::Resolve(gfx, "Sprite3D_Mask_PS.cso"));

			mask.AddBindable(tcb);
			mask.AddBindable(uvcb);

			outline.AddStep(std::move(mask));
		}
		AddTechnique(std::move(outline));
	}
	// shadow map technique
	{
		Technique map{ "ShadowMap", Chan::shadow, true };
		{
			Step draw("shadowMap");
			auto pvs = Bind::VertexShader::Resolve(gfx, "ShadowAlpha_VS.cso");
			draw.AddBindable(Bind::InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			draw.AddBindable(std::move(pvs));
			draw.AddBindable(Bind::PixelShader::Resolve(gfx, "ShadowAlpha_PS.cso"));

			draw.AddBindable(std::move(dynTex));
			draw.AddBindable(Bind::Sampler::Resolve(gfx));

			draw.AddBindable(std::move(tcb));
			draw.AddBindable(std::move(uvcb));

			draw.AddBindable(Bind::Rasterizer::Resolve(gfx, true));

			map.AddStep(std::move(draw));
		}
		AddTechnique(std::move(map));
	}
}