#include "CanvasStackCompositePass.h"
#include "Graphics.h"
#include "PixelShader.h"
#include "RenderTarget.h"
#include "Sink.h"
#include "Source.h"
#include "Blender.h"
#include "Stencil.h"
#include "Sampler.h"

using namespace Bind;

namespace Rgph
{
	CanvasStackCompositePass::CanvasStackCompositePass( std::string name,Graphics& gfx )
		:
		FullscreenPass( std::move( name ),gfx )
	{
		AddBind( PixelShader::Resolve( gfx,"CanvasStackComposite_PS.cso" ) );
		AddBind( Blender::Resolve( gfx,true ) );
		AddBind( Stencil::Resolve( gfx,Stencil::Mode::DepthOff ) );
		AddBind( Sampler::Resolve( gfx,Sampler::Type::Point,false,0u ) );

		AddBindSink<Bind::RenderTarget>( "mainCanvas" );
		AddBindSink<Bind::RenderTarget>( "minimapCanvas" );
		AddBindSink<Bind::RenderTarget>( "hudMaskCanvas" );

		RegisterSink( DirectBufferSink<RenderTarget>::Make( "renderTarget",renderTarget ) );
		RegisterSource( DirectBufferSource<RenderTarget>::Make( "renderTarget",renderTarget ) );
	}

	void CanvasStackCompositePass::Execute( Graphics& gfx ) const noxnd
	{
		FullscreenPass::Execute( gfx );
		gfx.ClearPixelShaderResourceRange( kFirstSrvSlot,kSrvCount );
	}
}
