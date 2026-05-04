#include "OffscreenCanvasPass.h"
#include "RenderTarget.h"
#include "Sink.h"
#include "Source.h"

namespace Rgph
{
	OffscreenCanvasPass::OffscreenCanvasPass( Graphics& gfx,std::string name,
		UINT width,UINT height,UINT srvBindSlot,
		std::array<float,4> clearRGBA )
		:
		Pass( std::move( name ) ),
		clearRGBA( clearRGBA )
	{
		canvas = std::make_shared<Bind::ShaderInputRenderTarget>( gfx,width,height,srvBindSlot );
		canvasIface = canvas;
		RegisterSource( DirectBufferSource<Bind::RenderTarget>::Make( "buffer",canvasIface ) );
		RegisterSource( DirectBindableSource<Bind::RenderTarget>::Make( "texture",canvasIface ) );
	}

	void OffscreenCanvasPass::Execute( Graphics& gfx ) const noxnd
	{
		canvasIface->Clear( gfx,clearRGBA );
	}

	std::shared_ptr<Bind::ShaderInputRenderTarget> OffscreenCanvasPass::SharedCanvas() noexcept
	{
		return canvas;
	}
}
