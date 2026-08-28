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
	/**
	 * @brief 局内管线：阴影、Lambert、天空盒、描边模糊、线框，末尾 UIPass。
	 */
	class GameRenderGraph : public RenderGraph
	{
	public:
		GameRenderGraph(Graphics& gfx);
		void RenderWindows(Graphics& gfx);
		void DumpShadowMap(Graphics& gfx, const std::string& path);
		void BindMainCamera(Camera& cam);
		void BindShadowCamera(Camera& cam);
		void Update(float dt) noxnd;
		void Interaction() override;
	private:
		void RenderKernelWindow(Graphics& gfx);
		void RenderShadowWindow(Graphics& gfx);
		// private functions
		void SetKernelGauss(int radius, float sigma) noxnd;
		void SetKernelBox(int radius) noxnd;
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
	};
}