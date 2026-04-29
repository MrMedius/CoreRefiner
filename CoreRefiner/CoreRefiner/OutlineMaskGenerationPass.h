#pragma once
#include "RenderQueuePass.h"
#include "Job.h"
#include <vector>
#include "NullPixelShader.h"
#include "VertexShader.h"
#include "Stencil.h"
#include "Rasterizer.h"

#include "Job.h"

class Graphics;

namespace Rgph
{
	class OutlineMaskGenerationPass : public RenderQueuePass
	{
	public:
		OutlineMaskGenerationPass( Graphics& gfx,std::string name )
			:
			RenderQueuePass( std::move( name ) )
		{
			using namespace Bind;
			RegisterSink( DirectBufferSink<Bind::DepthStencil>::Make( "depthStencil",depthStencil ) );
			RegisterSource( DirectBufferSource<Bind::DepthStencil>::Make( "depthStencil",depthStencil ) );
			AddBind( VertexShader::Resolve( gfx,"Solid_VS.cso" ) );
			AddBind( NullPixelShader::Resolve( gfx ) );
			AddBind( Stencil::Resolve( gfx,Stencil::Mode::Write ) );
			AddBind( Rasterizer::Resolve( gfx,false ) );
		}
		void Execute(Graphics& gfx) const noxnd override
		{
			EnsureDummyRT(gfx);
			// bind all states/shaders (this may override OM)
			BindAll(gfx);
			// bind dummyRT + DS to ensure that RT is not empty during Draw
			dummyRT->BindAsBuffer(gfx, depthStencil.get());
			// do jobs
			for (const auto& j : jobs)
			{
				j.Execute(gfx);
			}
		}
	private:
		void EnsureDummyRT(Graphics& gfx) const
		{
			// reset the size to fit fullscreen
			const UINT w = depthStencil->GetWidth();
			const UINT h = depthStencil->GetHeight();

			if (!dummyRT || dummyRT->GetWidth() != w || dummyRT->GetHeight() != h)
			{
				dummyRT = std::make_unique<Bind::ShaderInputRenderTarget>(gfx, w, h, 0u);
			}
		}
	private:
		mutable std::unique_ptr<Bind::ShaderInputRenderTarget> dummyRT;
	};
}