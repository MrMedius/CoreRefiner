#include "UserInterfaceRenderGraph.h"

#include "BufferClearPass.h"
#include "UIPass.h"

namespace Rgph
{
	UserInterfaceRenderGraph::UserInterfaceRenderGraph(Graphics& gfx)
		:
		RenderGraph(gfx)
	{
		{
			auto pass = std::make_unique<BufferClearPass>("clearRT");
			pass->SetSinkLinkage("buffer", "$.backbuffer");
			AppendPass(std::move(pass));
		}
		{
			auto pass = std::make_unique<UIPass>(gfx, "ui");
			pass->SetSinkLinkage("renderTarget", "clearRT.buffer");
			AppendPass(std::move(pass));
		}
		SetSinkTarget("backbuffer", "ui.renderTarget");

		Finalize();
	}
}