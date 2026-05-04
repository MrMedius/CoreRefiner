#pragma once
#include "Win.h"
#include "Pass.h"
#include <array>
#include <memory>

namespace Bind
{
	class RenderTarget;
	class ShaderInputRenderTarget;
}

namespace Rgph
{
	/**
	 * @brief Holds a ShaderInput RT, clears it each frame, exports buffer + texture sources.
	 */
	class OffscreenCanvasPass : public Pass
	{
	public:
		OffscreenCanvasPass( Graphics& gfx,std::string name,
			UINT width,UINT height,UINT srvBindSlot,
			std::array<float,4> clearRGBA ) noxnd;
		void Execute( Graphics& gfx ) const noxnd override;
		std::shared_ptr<Bind::ShaderInputRenderTarget> SharedCanvas() noexcept;
	private:
		std::shared_ptr<Bind::ShaderInputRenderTarget> canvas;
		/** @brief Alias as RenderTarget (same GPU resource as canvas). */
		std::shared_ptr<Bind::RenderTarget> canvasIface;
		std::array<float,4> clearRGBA;
	};
}
