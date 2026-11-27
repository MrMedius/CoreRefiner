#include "UiToggle.h"
#include "FocusManager.h"

namespace Ui
{
	UiToggle::UiToggle(const FocusHandle focusHandle, UiRect bounds)
		:
		focusHandle_(focusHandle),
		bounds_(bounds)
	{}

	void UiToggle::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
			pointerPress_.Reset();
	}

	void UiToggle::ApplyIsOn_(const bool on, const bool notify) noexcept
	{
		if (isOn_ == on)
			return;

		isOn_ = on;
		if (notify && onValueChanged_)
			onValueChanged_(isOn_);
	}

	void UiToggle::SetIsOn(const bool on, const bool notify) noexcept
	{
		ApplyIsOn_(on, notify);
	}

	bool UiToggle::Toggle() noexcept
	{
		if (!enabled_)
			return isOn_;

		ApplyIsOn_(!isOn_, true);
		return isOn_;
	}

	bool UiToggle::IsPointerOver(const UiInputFrame& frame) const noexcept
	{
		if (!Input::ClientPointValid(frame.pointer))
			return false;
		return bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	void UiToggle::RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept
	{
		visualPhase_ = ComputeStandardPhase(
			enabled_,
			pointerPress_.ShouldShowPressed(frame.pointer.primaryDown),
			IsPointerOver(frame),
			focus.IsFocused(focusHandle_));
	}

	void UiToggle::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		if (!enabled_)
		{
			pointerPress_.Reset();
			pointerPress_.SyncFrame(frame.pointer.primaryDown);
			RecomputeVisualPhase(frame, focus);
			return;
		}

		if (focus.IsFocused(focusHandle_) && frame.action.confirmPressed)
			Toggle();

		const bool over = IsPointerOver(frame);
		pointerPress_.TryBeginPress(frame.pointer.primaryDown, over);
		const bool releaseInside = Input::ClientPointValid(frame.pointer)
			&& bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
		if (pointerPress_.TryCompletePress(frame.pointer.primaryDown, releaseInside))
			Toggle();

		pointerPress_.SyncFrame(frame.pointer.primaryDown);
		RecomputeVisualPhase(frame, focus);
	}

	void UiToggle::ResetPointerInteraction() noexcept
	{
		pointerPress_.Reset();
	}
}