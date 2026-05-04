#pragma once
#include "RenderGraph.h"
#include <memory>
#include "ConstantBuffersEx.h"
#include "Camera.h"

class Graphics;
class Camera;
namespace Bind
{
	class Bindable;
	class RenderTarget;
	class ShadowSampler;
	class ShadowRasterizer;
}

namespace Rgph
{
	class InGameRenderGraph : public RenderGraph
	{
	public:
		InGameRenderGraph( Graphics& gfx );
		void RenderWindows(Graphics& gfx);
		void DumpShadowMap(Graphics& gfx, const std::string& path);
		void BindMainCamera(Camera& cam);
		void BindShadowCamera(Camera& cam);
		void Update(float dt) noxnd;
		void Interaction() override;
		/**
		 * @brief Recreate offscreen canvas textures when LOGICAL_CANVAS_* dimensions change.
		 */
		void RebuildLogicalCanvasses( Graphics& gfx ) noxnd;
		bool IsCanvasCompositionEnabled() const noexcept { return canvasCompositionEnabled; }
		void SetCanvasCompositionEnabled( bool enabled ) noexcept { canvasCompositionEnabled = enabled; }
#ifndef NDEBUG
		void RunCanvasValidationHeartbeat() noexcept;
#endif
	private:
		void RenderKernelWindow(Graphics& gfx);
		void RenderShadowWindow(Graphics& gfx);
		void RenderCanvasWindow( Graphics& gfx );
		// private functions
		void SetKernelGauss( int radius,float sigma ) noxnd;
		void SetKernelBox( int radius ) noxnd;
		// private data
		enum class KernelType
		{
			Gauss,
			Box,
		} kernelType = KernelType::Gauss;
		static constexpr int maxRadius = 100;
		int radius = 50; // 10;
		float sigma = 50.0f; // 10.0f;
		std::shared_ptr<Bind::CachingPixelConstantBufferEX> blurKernel;
		std::shared_ptr<Bind::CachingPixelConstantBufferEX> blurDirection;
		std::shared_ptr<Bind::CachingPixelConstantBufferEX> colorBlender;
		/** @brief Placeholder flag; switching pipeline at runtime requires graph rebuild. */
		bool canvasCompositionEnabled = true;
#ifndef NDEBUG
		unsigned validationFrameCounter = 0;
#endif
	};
}