#pragma once
#include "RenderQueuePass.h"
#include "Sink.h"
#include "Source.h"
#include "Stencil.h"
#include "Blender.h"
#include "Rasterizer.h"

class Graphics;

namespace Bind
{
    class RenderTarget;
}

namespace Rgph
{
    class UIPass : public RenderQueuePass
    {
    public:
        UIPass(Graphics& gfx, std::string name)
            :
			RenderQueuePass(std::move(name))
        {
            using namespace Bind;
            RegisterSink(DirectBufferSink<RenderTarget>::Make("renderTarget", renderTarget));
            RegisterSource(DirectBufferSource<RenderTarget>::Make("renderTarget", renderTarget));
            
            AddBind(Stencil::Resolve(gfx, Stencil::Mode::Off));
            AddBind(Blender::Resolve(gfx, true));
            AddBind(Rasterizer::Resolve(gfx, true));
        }
        void Execute(Graphics& gfx) const noxnd override
        {
            assert(renderTarget);

            renderTarget->BindAsBuffer(gfx);
            RenderQueuePass::Execute(gfx);
        }
    };
}