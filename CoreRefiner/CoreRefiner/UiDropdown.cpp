#include "UiDropdown.h"

#include "FocusManager.h"

#include <algorithm>

namespace Ui
{
	namespace
	{
		[[nodiscard]] bool ClientPointValid(const UiPointerPayload& p) noexcept
		{
			return p.insideLogicalSurface;
		}
	}

	UiDropdown::UiDropdown(const FocusHandle focusHandle, UiRect headerBounds)
		:
		focusHandle_(focusHandle),
		headerBounds_(headerBounds)
	{
		options_ = {
			DropdownOption{ .label = "Low" },
			DropdownOption{ .label = "Medium" },
			DropdownOption{ .label = "High" }
		};
		selectedIndex_ = 0;
	}

	void UiDropdown::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = false;
		}
	}

	void UiDropdown::SetOptions(std::vector<DropdownOption> options)
	{
		options_ = std::move(options);
		if (options_.empty())
			selectedIndex_ = -1;
		else
			selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(options_.size()) - 1);
	}

	void UiDropdown::SetSelectedIndex(const int index, const bool notify) noexcept
	{
		if (options_.empty())
		{
			selectedIndex_ = -1;
			return;
		}

		const int clamped = std::clamp(index, 0, static_cast<int>(options_.size()) - 1);
		if (selectedIndex_ == clamped)
			return;

		selectedIndex_ = clamped;
		if (notify && onValueChanged_)
			onValueChanged_(selectedIndex_, options_[static_cast<size_t>(selectedIndex_)].label);
	}

	std::string UiDropdown::GetSelectedLabel() const
	{
		if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(options_.size()))
			return {};
		return options_[static_cast<size_t>(selectedIndex_)].label;
	}

	bool UiDropdown::IsPointerOverHeader_(const UiInputFrame& frame) const noexcept
	{
		if (!ClientPointValid(frame.pointer))
			return false;
		return headerBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	void UiDropdown::RecomputeHeaderPhase_(
		const UiInputFrame& frame,
		const FocusManager& focus) noexcept
	{
		if (!enabled_)
		{
			headerPhase_ = UiVisualPhase::Disabled;
			return;
		}

		const bool focused = IsPointerOverHeader_(frame) || focus.IsFocused(focusHandle_);
		const bool pressVisual = trackingPointerPress_ && frame.pointer.primaryDown;
		if (pressVisual)
			headerPhase_ = UiVisualPhase::Pressed;
		else if (focused)
			headerPhase_ = UiVisualPhase::Focused;
		else
			headerPhase_ = UiVisualPhase::Normal;
	}

	void UiDropdown::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = frame.pointer.primaryDown;
			RecomputeHeaderPhase_(frame, focus);
			return;
		}

		const bool overHeader = IsPointerOverHeader_(frame);
		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_ && overHeader)
			trackingPointerPress_ = true;

		if (!frame.pointer.primaryDown && trackingPointerPress_)
		{
			trackingPointerPress_ = false;
			const bool releaseInside = ClientPointValid(frame.pointer)
				&& headerBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
			if (releaseInside)
			{
				// Step 4 将实现展开；Step 3 仅更新视觉相位
			}
		}

		pointerWasDownLastFrame_ = frame.pointer.primaryDown;
		RecomputeHeaderPhase_(frame, focus);
	}

	void UiDropdown::ResetPointerInteraction() noexcept
	{
		trackingPointerPress_ = false;
		pointerWasDownLastFrame_ = false;
	}
}
