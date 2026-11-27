#pragma once

#include <cstddef>

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class IUiView
	{
	public:
		virtual ~IUiView() = default;

		virtual void LinkTechniques(Rgph::RenderGraph& rg) = 0;
		virtual void Submit(std::size_t channelMask) const = 0;
	};
}
