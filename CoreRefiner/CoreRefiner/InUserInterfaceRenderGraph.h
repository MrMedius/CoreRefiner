#pragma once
#include "RenderGraph.h"

class Graphics;

namespace Rgph
{
	class InUserInterfaceRenderGraph : public RenderGraph
	{
	public:
		InUserInterfaceRenderGraph(Graphics& gfx);
		void Interaction() override {};
	};
}