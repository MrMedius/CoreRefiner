#pragma once
#include "RenderQueuePass.h"
#include "BindableCommon.h"
#include "Sink.h"
#include "Source.h"
#include "Camera.h"
#include "DepthStencil.h"

class Graphics;

namespace Rgph
{
	class SkyboxPass : public RenderQueuePass
	{
	public:
		SkyboxPass(Graphics& gfx, std::string name)
			:
			RenderQueuePass(std::move(name))
		{
			using namespace Bind;
			RegisterSink(DirectBufferSink<RenderTarget>::Make("renderTarget", renderTarget));
			RegisterSink(DirectBufferSink<DepthStencil>::Make("depthStencil", depthStencil));

			AddBind(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));
			AddBind(Sampler::Resolve(gfx, Sampler::Type::Bilinear));
			AddBind(Rasterizer::Resolve(gfx, true));

			RegisterSource(DirectBufferSource<RenderTarget>::Make("renderTarget", renderTarget));
			RegisterSource(DirectBufferSource<DepthStencil>::Make("depthStencil", depthStencil));
		}
		void BindMainCamera(const Camera& cam) noexcept
		{
			pMainCamera = &cam;
		}
		void Execute(Graphics& gfx) const noxnd override
		{
			assert(pMainCamera);
			pMainCamera->BindToGraphics(gfx);

			RenderQueuePass::Execute(gfx);
		}
	private:
		const Camera* pMainCamera = nullptr;
	};
}