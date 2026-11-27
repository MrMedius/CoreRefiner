#include "UiButton.h"
#include "FocusManager.h"

namespace Ui
{
	UiButton::UiButton(FocusHandle focusHandle, UiRect bounds)
		:
		focusHandle_(focusHandle),
		bounds_(bounds)
	{}

	void UiButton::SetEnabled(bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			pointerPress_.Reset();
			trackingConfirmPress_ = false;
		}
	}

	bool UiButton::IsPointerOver(const UiInputFrame& frame) const noexcept
	{
		if (!Input::ClientPointValid(frame.pointer))
			return false;
		return bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	void UiButton::RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept
	{
		const bool pressVisual = pointerPress_.ShouldShowPressed(frame.pointer.primaryDown)
			|| (trackingConfirmPress_ && frame.action.confirmDown);
		visualPhase_ = ComputeStandardPhase(
			enabled_,
			pressVisual,
			IsPointerOver(frame),
			focus.IsFocused(focusHandle_));
	}

	void UiButton::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		if (!enabled_)
		{
			pointerPress_.Reset();
			trackingConfirmPress_ = false;
			pointerPress_.SyncFrame(frame.pointer.primaryDown);
			RecomputeVisualPhase(frame, focus);
			return;
		}

		const bool focused = focus.IsFocused(focusHandle_);
		if (frame.action.confirmPressed && focused)
			trackingConfirmPress_ = true;
		if (frame.action.confirmReleased)
		{
			if (trackingConfirmPress_)
			{
				trackingConfirmPress_ = false;
				if (focused && onClick_)
					onClick_();
			}
		}
		else if (trackingConfirmPress_ && !frame.action.confirmDown)
			trackingConfirmPress_ = false;
		if (trackingConfirmPress_ && !focused)
			trackingConfirmPress_ = false;

		const bool over = IsPointerOver(frame);
		pointerPress_.TryBeginPress(frame.pointer.primaryDown, over);
		const bool releaseInside = Input::ClientPointValid(frame.pointer)
			&& bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
		if (pointerPress_.TryCompletePress(frame.pointer.primaryDown, releaseInside) && onClick_)
			onClick_();

		pointerPress_.SyncFrame(frame.pointer.primaryDown);
		RecomputeVisualPhase(frame, focus);
	}

	void UiButton::ResetPointerInteraction() noexcept
	{
		pointerPress_.Reset();
		trackingConfirmPress_ = false;
	}
}