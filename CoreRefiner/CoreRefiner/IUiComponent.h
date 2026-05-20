#pragma once

#include "FocusTypes.h"
#include "UiInputFrame.h"

#include <cstddef>

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class FocusManager;

	class IUiComponent
	{
	public:
		virtual ~IUiComponent() = default;

		virtual void Update(const UiInputFrame& frame, const FocusManager& focus) = 0;
		virtual void SyncView() = 0;
		virtual void LinkTechniques(Rgph::RenderGraph& rg) = 0;
		virtual void Submit(const std::size_t channelMask) const = 0;

		[[nodiscard]] virtual FocusHandle GetFocusHandle() const noexcept = 0;
		virtual void ResetPointerInteraction() noexcept = 0;
		[[nodiscard]] virtual bool IsFocusable() const noexcept = 0;
	};
}
