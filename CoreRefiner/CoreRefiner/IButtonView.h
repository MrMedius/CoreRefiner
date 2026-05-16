#pragma once

#include "ButtonViewModel.h"

#include <cstddef>

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class IButtonView
	{
	public:
		virtual ~IButtonView() = default;

		virtual void SyncFrom(const ButtonViewModel& vm) = 0;
		virtual void LinkTechniques(Rgph::RenderGraph& rg) = 0;
		virtual void Submit(std::size_t channelMask) const = 0;
	};
}