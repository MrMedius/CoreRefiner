#pragma once
#include "FullscreenPass.h"

class Graphics;

namespace Bind
{
	class RenderTarget;
}

namespace Rgph
{
	/**
	 * @brief Samples main canvas + two sub-canvases, alpha-composites to target RT (usually backbuffer).
	 * PS must bind t10-t12 matching ShaderInput slots and sampler s0.
	 */
	class CanvasStackCompositePass : public FullscreenPass
	{
	public:
		CanvasStackCompositePass( std::string name,Graphics& gfx );
		void Execute( Graphics& gfx ) const noxnd override;
	private:
		static constexpr UINT kFirstSrvSlot = 10u;
		static constexpr UINT kSrvCount = 3u;
	};
}
