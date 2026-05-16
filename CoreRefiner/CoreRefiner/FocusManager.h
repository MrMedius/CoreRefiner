#pragma once

#include "FocusTypes.h"
#include "UiInputFrame.h"

#include <cstddef>
#include <vector>

namespace Ui
{
	class FocusManager
	{
	public:
		FocusManager() = default;

		void SetTabOrder(std::vector<FocusHandle> order);

		void RegisterTabStop(FocusHandle h);

		void UnregisterTabStop(FocusHandle h);

		void ClearTabOrder() noexcept;

		void ClearFocus() noexcept;

		void RequestFocus(FocusHandle h) noexcept;

		FocusHandle Focused() const noexcept { return focused_; }

		bool IsFocused(FocusHandle h) const noexcept
		{
			return h != kInvalidFocusHandle && focused_ == h;
		}

		void ApplyNavigation(const UiInputFrame& frame);

		void FocusNext() noexcept;

		void FocusPrev() noexcept;

	private:
		std::ptrdiff_t FindIndex(FocusHandle h) const noexcept;
		void FocusAtIndex(std::size_t index) noexcept;

		std::vector<FocusHandle> tabOrder_{};
		FocusHandle focused_ = kInvalidFocusHandle;
	};
}