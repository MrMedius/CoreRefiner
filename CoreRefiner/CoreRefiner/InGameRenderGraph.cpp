#include "InGameRenderGraph.h"
#include "Graphics.h"
#include "Sink.h"
#include "Source.h"
#include "RenderTarget.h"
#include "DynamicConstant.h"
#include "PerfLog.h"
#include "imgui/imgui.h"
#include "Math.h"
#include <algorithm>
#include <array>
#include <filesystem>

#include "BufferClearPass.h"
#include "ShadowMappingPass.h"
#include "LambertianPass.h"
#include "SkyboxPass.h"
#include "LambertianTransparentPass.h"
#include "OutlineMaskGenerationPass.h"
#include "BlurOutlineDrawingPass.h"
#include "HorizontalBlurPass.h"
#include "VerticalBlurPass.h"
#include "WireframePass.h"
#include "UIPass.h"
#include "OffscreenCanvasPass.h"
#include "CanvasStackCompositePass.h"

namespace Rgph
{
	InGameRenderGraph::InGameRenderGraph( Graphics& gfx )
		:
		RenderGraph( gfx )
	{
		{
			auto pass = std::make_unique<BufferClearPass>( "clearRT" );
			pass->SetSinkLinkage( "buffer","$.backbuffer" );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<BufferClearPass>( "clearDS" );
			pass->SetSinkLinkage( "buffer","$.masterDepth" );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<ShadowMappingPass>(gfx, "shadowMap");
			AppendPass(std::move(pass));
		}
		{
			auto pass = std::make_unique<LambertianPass>( gfx,"lambertian" );
			pass->SetSinkLinkage( "shadowMap","shadowMap.map" );
			pass->SetSinkLinkage( "renderTarget","clearRT.buffer" );
			pass->SetSinkLinkage( "depthStencil","clearDS.buffer" );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<SkyboxPass>(gfx, "skybox");
			pass->SetSinkLinkage( "renderTarget", "lambertian.renderTarget" );
			pass->SetSinkLinkage( "depthStencil","lambertian.depthStencil" );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<LambertianTransparentPass>(gfx, "lambertianTrans");
			pass->SetSinkLinkage("shadowMap", "shadowMap.map");
			pass->SetSinkLinkage("renderTarget", "skybox.renderTarget");
			pass->SetSinkLinkage("depthStencil", "skybox.depthStencil");
			AppendPass(std::move(pass));
		}
		{
			auto pass = std::make_unique<OutlineMaskGenerationPass>(gfx, "outlineMask");
			pass->SetSinkLinkage("depthStencil", "lambertianTrans.depthStencil");
			AppendPass(std::move(pass));
		}
		// setup blur constant buffers
		{
			{
				Dcb::RawLayout l;
				l.Add<Dcb::Integer>( "nTaps" );
				l.Add<Dcb::Array>( "coefficients" );
				l["coefficients"].Set<Dcb::Float>( maxRadius * 2 + 1 );
				Dcb::Buffer buf{ std::move( l ) };
				blurKernel = std::make_shared<Bind::CachingPixelConstantBufferEX>( gfx,buf,0 );
				SetKernelGauss( radius,sigma );
				AddGlobalSource( DirectBindableSource<Bind::CachingPixelConstantBufferEX>::Make( "blurKernel", blurKernel) );
			}
			{
				Dcb::RawLayout l;
				l.Add<Dcb::Bool>( "isHorizontal" );
				Dcb::Buffer buf{ std::move( l ) };
				blurDirection = std::make_shared<Bind::CachingPixelConstantBufferEX>( gfx,buf,1 );
				AddGlobalSource( DirectBindableSource<Bind::CachingPixelConstantBufferEX>::Make( "blurDirection",blurDirection ) );
			}
		}
		// setup edge color blend constant buffers 
		{ 
			{ 
				Dcb::RawLayout l; l.Add<Dcb::Float3>("colorBlending"); 
				Dcb::Buffer buf{ std::move(l) }; 
				buf["colorBlending"] = DirectX::XMFLOAT3{ 139.0f,0.0f,0.0f };
				colorBlender = std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 0); 
				AddGlobalSource(DirectBindableSource<Bind::CachingPixelConstantBufferEX>::Make("colorBlender", colorBlender)); 
			} 
		}
		{
			auto pass = std::make_unique<BlurOutlineDrawingPass>( gfx,"outlineDraw",gfx.GetWidth(),gfx.GetHeight() );
			pass->SetSinkLinkage("blending", "$.colorBlender");
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<HorizontalBlurPass>( "horizontal",gfx,gfx.GetWidth(),gfx.GetHeight() );
			pass->SetSinkLinkage( "scratchIn","outlineDraw.scratchOut" );
			pass->SetSinkLinkage( "kernel","$.blurKernel" );
			pass->SetSinkLinkage( "direction","$.blurDirection" );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<VerticalBlurPass>( "vertical",gfx );
			pass->SetSinkLinkage( "renderTarget","lambertianTrans.renderTarget" );
			pass->SetSinkLinkage( "depthStencil","outlineMask.depthStencil" );
			pass->SetSinkLinkage( "scratchIn","horizontal.scratchOut" );
			pass->SetSinkLinkage( "kernel","$.blurKernel" );
			pass->SetSinkLinkage( "direction","$.blurDirection" );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<WireframePass>( gfx,"wireframe" );
			pass->SetSinkLinkage( "renderTarget","vertical.renderTarget" );
			pass->SetSinkLinkage( "depthStencil","vertical.depthStencil" );
			AppendPass( std::move( pass ) );
		}
		{
			const UINT lcw = Graphics::LogicalCanvasWidth();
			const UINT lch = Graphics::LogicalCanvasHeight();
			auto pass = std::make_unique<OffscreenCanvasPass>(
				gfx,"mainCanvasTarget",lcw,lch,10u,
				std::array<float,4>{ 0.0f,0.0f,0.0f,0.0f } );
			AppendPass( std::move( pass ) );
		}
		{
			const UINT lcw = Graphics::LogicalCanvasWidth();
			const UINT lch = Graphics::LogicalCanvasHeight();
			auto pass = std::make_unique<OffscreenCanvasPass>(
				gfx,"minimapCanvasTarget",std::max( 1u,lcw / 2u ),std::max( 1u,lch / 2u ),11u,
				std::array<float,4>{ 0.35f,0.0f,0.45f,0.5f } );
			AppendPass( std::move( pass ) );
		}
		{
			const UINT lcw = Graphics::LogicalCanvasWidth();
			const UINT lch = Graphics::LogicalCanvasHeight();
			auto pass = std::make_unique<OffscreenCanvasPass>(
				gfx,"hudMaskCanvasTarget",lcw,lch,12u,
				std::array<float,4>{ 0.0f,0.0f,0.0f,0.0f } );
			AppendPass( std::move( pass ) );
		}
		{
			auto pass = std::make_unique<UIPass>(gfx, "ui");
			pass->SetSinkLinkage("renderTarget", "mainCanvasTarget.buffer");
			AppendPass(std::move(pass));
		}
		{
			auto pass = std::make_unique<CanvasStackCompositePass>("canvasComposite",gfx );
			pass->SetSinkLinkage( "renderTarget","$.backbuffer" );
			pass->SetSinkLinkage( "mainCanvas","mainCanvasTarget.texture" );
			pass->SetSinkLinkage( "minimapCanvas","minimapCanvasTarget.texture" );
			pass->SetSinkLinkage( "hudMaskCanvas","hudMaskCanvasTarget.texture" );
			AppendPass( std::move( pass ) );
		}
		SetSinkTarget( "backbuffer","canvasComposite.renderTarget" );

		Finalize();
	}

	void InGameRenderGraph::SetKernelGauss( int radius,float sigma ) noxnd
	{
		assert( radius <= maxRadius );
		auto k = blurKernel->GetBuffer();
		const int nTaps = radius * 2 + 1;
		k["nTaps"] = nTaps;
		float sum = 0.0f;
		for( int i = 0; i < nTaps; i++ )
		{
			const auto x = float( i - radius );
			const auto g = gauss( x,sigma );
			sum += g;
			k["coefficients"][i] = g;
		}
		for( int i = 0; i < nTaps; i++ )
		{
			k["coefficients"][i] = (float)k["coefficients"][i] / sum;
		}
		blurKernel->SetBuffer( k );
	}

	void InGameRenderGraph::SetKernelBox(int radius) noxnd
	{
		assert(radius <= maxRadius);
		auto k = blurKernel->GetBuffer();
		const int nTaps = radius * 2 + 1;
		k["nTaps"] = nTaps;
		const float c = 1.0f / nTaps;
		for (int i = 0; i < nTaps; i++)
		{
			k["coefficients"][i] = c;
		}
		blurKernel->SetBuffer(k);
	}

	void InGameRenderGraph::RenderWindows(Graphics& gfx)
	{
		RenderShadowWindow(gfx);
		RenderKernelWindow(gfx);
		RenderCanvasWindow(gfx);
	}

	void InGameRenderGraph::RenderCanvasWindow( Graphics& gfx )
	{
		if( ImGui::Begin( "Canvas pipeline" ) )
		{
			ImGui::Text( "Logical: %ux%u",Graphics::LogicalCanvasWidth(),Graphics::LogicalCanvasHeight() );
			ImGui::Checkbox( "Composition flag (reserved)",&canvasCompositionEnabled );
			if( ImGui::Button( "Rebuild canvasses" ) )
				RebuildLogicalCanvasses( gfx );
		}
		ImGui::End();
	}

	void InGameRenderGraph::RenderKernelWindow(Graphics& gfx)
	{
		if (ImGui::Begin("Kernel"))
		{
			bool filterChanged = false;
			{
				const char* items[] = { "Gauss","Box" };
				static const char* curItem = items[0];
				if (ImGui::BeginCombo("Filter Type", curItem))
				{
					for (int n = 0; n < std::size(items); n++)
					{
						const bool isSelected = (curItem == items[n]);
						if (ImGui::Selectable(items[n], isSelected))
						{
							filterChanged = true;
							curItem = items[n];
							if (curItem == items[0])
							{
								kernelType = KernelType::Gauss;
							}
							else if (curItem == items[1])
							{
								kernelType = KernelType::Box;
							}
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}

			bool radChange = ImGui::SliderInt("Radius", &radius, 0, maxRadius);
			bool sigChange = ImGui::SliderFloat("Sigma", &sigma, 0.1f, 100.0f);
			if (radChange || sigChange || filterChanged)
			{
				if (kernelType == KernelType::Gauss)
				{
					SetKernelGauss(radius, sigma);
				}
				else if (kernelType == KernelType::Box)
				{
					SetKernelBox(radius);
				}
			}
		}
		ImGui::End();
	}
	void Rgph::InGameRenderGraph::RenderShadowWindow(Graphics& gfx) 
	{
		if (ImGui::Begin("Shadow"))
		{
			if (ImGui::Button("Dump Cubemap"))
			{
				namespace fs = std::filesystem;
				fs::create_directories("Dumps");

				DumpShadowMap(gfx, "Dumps\\shadow_");
			}
		}
		ImGui::End();
	}
	void Rgph::InGameRenderGraph::BindMainCamera(Camera& cam)
	{
		dynamic_cast<LambertianPass&>(FindPassByName("lambertian")).BindMainCamera(cam);
		dynamic_cast<SkyboxPass&>(FindPassByName("skybox")).BindMainCamera(cam);
		dynamic_cast<LambertianTransparentPass&>(FindPassByName("lambertianTrans")).BindMainCamera(cam);
	}
	void Rgph::InGameRenderGraph::BindShadowCamera(Camera& cam)
	{
		dynamic_cast<ShadowMappingPass&>(FindPassByName("shadowMap")).BindShadowCamera(cam);
		dynamic_cast<LambertianPass&>(FindPassByName( "lambertian")).BindShadowCamera(cam);
		dynamic_cast<LambertianTransparentPass&>(FindPassByName("lambertianTrans")).BindShadowCamera(cam);
	}
	void Rgph::InGameRenderGraph::DumpShadowMap(Graphics& gfx, const std::string& path)
	{
		dynamic_cast<ShadowMappingPass&>(FindPassByName("shadowMap")).DumpShadowMap(gfx, path);
	}

	void InGameRenderGraph::Update(float dt) noxnd
	{
		if (sigma > 10.0f)
		{
			sigma -= dt * 30.0f;
			radius = static_cast<int>(sigma);

			if (kernelType == KernelType::Gauss)
				SetKernelGauss(radius, sigma);
			else
				SetKernelBox(radius);
		}
	}

	void Rgph::InGameRenderGraph::Interaction()
	{
		if (sigma < 50.0f)
		{
			sigma += 10.0f;

		}
	}

	void InGameRenderGraph::RebuildLogicalCanvasses( Graphics& gfx ) noxnd
	{
		auto& mainPass = dynamic_cast<OffscreenCanvasPass&>(FindPassByName( "mainCanvasTarget" ));
		auto& miniPass = dynamic_cast<OffscreenCanvasPass&>(FindPassByName( "minimapCanvasTarget" ));
		auto& hudPass = dynamic_cast<OffscreenCanvasPass&>(FindPassByName( "hudMaskCanvasTarget" ));
		const UINT lcw = Graphics::LogicalCanvasWidth();
		const UINT lch = Graphics::LogicalCanvasHeight();
		mainPass.SharedCanvas()->Resize( gfx,lcw,lch );
		miniPass.SharedCanvas()->Resize( gfx,std::max( 1u,lcw / 2u ),std::max( 1u,lch / 2u ) );
		hudPass.SharedCanvas()->Resize( gfx,lcw,lch );
		gfx.ClearPixelShaderResourceRange( 10u,3u );
	}

#ifndef NDEBUG
	void InGameRenderGraph::RunCanvasValidationHeartbeat() noexcept
	{
		if ((++validationFrameCounter % 600u) != 0u)
			return;
		PerfLog::Info(
			std::string( "[Canvas] heartbeat logical=" ) +
			std::to_string( Graphics::LogicalCanvasWidth() ) + 'x' +
			std::to_string( Graphics::LogicalCanvasHeight() ) +
			" main+minimap+hud-composite" );
	}
#endif
}