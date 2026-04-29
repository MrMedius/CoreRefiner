#pragma once
#include "RenderQueuePass.h"
#include "Sink.h"
#include "Source.h"
#include "Stencil.h"
#include "Camera.h"
#include "DepthStencil.h"
#include "ShadowCameraCbuf.h"
#include "ShadowSampler.h"
#include "Sampler.h"

namespace Rgph
{
    class LambertianTransparentPass : public RenderQueuePass
    {
    public:
        LambertianTransparentPass(Graphics& gfx, std::string name)
            :
            RenderQueuePass(std::move(name)),
            pShadowCBuf{ std::make_shared<Bind::ShadowCameraCbuf>(gfx) }
        {
            using namespace Bind;

            AddBind(pShadowCBuf);

            RegisterSink(DirectBufferSink<RenderTarget>::Make("renderTarget", renderTarget));
            RegisterSink(DirectBufferSink<DepthStencil>::Make("depthStencil", depthStencil));
            AddBindSink<Bind::Bindable>("shadowMap");

            AddBind(std::make_shared<Bind::ShadowSampler>(gfx));
            AddBind(std::make_shared<Bind::Sampler>(gfx, Bind::Sampler::Type::Anisotropic, false, 2));

            RegisterSource(DirectBufferSource<RenderTarget>::Make("renderTarget", renderTarget));
            RegisterSource(DirectBufferSource<DepthStencil>::Make("depthStencil", depthStencil));

            // Transparent => Read Depth Only, NO writing
            AddBind(Stencil::Resolve(gfx, Stencil::Mode::DepthOnly));
        }

        void BindMainCamera(const Camera& cam) noexcept
        {
            pMainCamera = &cam;
        }
        void BindShadowCamera(const Camera& cam) noexcept
        {
            pShadowCBuf->SetCamera(&cam);
        }

        void Execute(Graphics& gfx) const noxnd override
        {
            assert(pMainCamera);
            pShadowCBuf->Update(gfx);
            pMainCamera->BindToGraphics(gfx);
            RenderQueuePass::Execute(gfx);
        }

    private:
        std::shared_ptr<Bind::ShadowCameraCbuf> pShadowCBuf;
        const Camera* pMainCamera = nullptr;
    };
}
