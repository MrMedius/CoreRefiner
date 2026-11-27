#include "Canvas3D.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include "CanvasTexture.h"
#include "Channels.h"

Canvas3D::Canvas3D(Graphics& gfx, unsigned width, unsigned height)
	:
	Canvas(width, height)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Plane::Make();
	model.SetNormalsIndependentFlat();
	const auto geometryTag = std::string("$canvas3d.") + std::to_string(width) + "x" + std::to_string(height);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<TransformCbuf>(gfx);
	auto canvasTex = std::make_shared<CanvasTexture>(gfx);

	{
		Technique shade("Shade", Chan::main);
		{
			Step only("lambertian");

			only.AddBindable(canvasTex);
			only.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));

			auto pvs = VertexShader::Resolve(gfx, "Canvas3D_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));
			
			only.AddBindable(PixelShader::Resolve(gfx, "Canvas3D_PS.cso"));

			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("specularColor");
			lay.Add<Dcb::Float>("specularWeight");
			lay.Add<Dcb::Float>("specularGloss");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["specularColor"] = dx::XMFLOAT3{ 1.0f, 1.0f, 1.0f };
			buf["specularWeight"] = 0.1f;
			buf["specularGloss"] = 20.0f;
			only.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));
			
			only.AddBindable(Rasterizer::Resolve(gfx, true));
			
			only.AddBindable(tcb);
			
			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
	}
	//{
	//	Technique outline("Outline", Chan::main);
	//	{
	//		Step mask("outlineMask");
	//		
	//		auto pvs = VertexShader::Resolve(gfx, "Sprite3D_Mask_VS.cso");
	//		mask.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
	//		mask.AddBindable(std::move(pvs));
	//		
	//		mask.AddBindable(canvasTex);
	//		mask.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));
	//		
	//		mask.AddBindable(PixelShader::Resolve(gfx, "Sprite3D_Mask_PS.cso"));
	//		
	//		mask.AddBindable(tcb);
	//		
	//		outline.AddStep(std::move(mask));
	//	}
	//	AddTechnique(std::move(outline));
	//}
	// shadow map technique
	{
		Technique map{ "ShadowMap", Chan::shadow, true };
		{
			Step draw("shadowMap");
			auto pvs = VertexShader::Resolve(gfx, "ShadowAlpha_VS.cso");
			draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			draw.AddBindable(std::move(pvs));
			draw.AddBindable(PixelShader::Resolve(gfx, "ShadowAlpha_PS.cso"));
			
			draw.AddBindable(canvasTex);
			draw.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));
			
			draw.AddBindable(tcb);
			
			draw.AddBindable(Rasterizer::Resolve(gfx, true));
			
			map.AddStep(std::move(draw));
		}
		AddTechnique(std::move(map));
	}
}