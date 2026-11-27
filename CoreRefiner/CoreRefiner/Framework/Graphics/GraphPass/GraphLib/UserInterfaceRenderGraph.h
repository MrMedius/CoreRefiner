#pragma once
#include "RenderGraph.h"

class Graphics;

namespace Rgph
{
	class UserInterfaceRenderGraph : public RenderGraph
	{
	public:
		UserInterfaceRenderGraph(Graphics& gfx);
		void Interaction() override {};
	};
}