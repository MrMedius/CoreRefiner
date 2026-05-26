#include "UiButton.h"
#include "FocusManager.h"

namespace Ui
{
	namespace
	{
		constexpr bool ClientPointValid(const UiPointerPayload& p) noexcept
		{
			return p.insideLogicalSurface;
		}
	}

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
			trackingPointerPress_ = false;
			trackingConfirmPress_ = false;
			pointerWasDownLastFrame_ = false;
		}
	}

	bool UiButton::IsPointerOver(const UiInputFrame& frame) const noexcept
	{
		if (!ClientPointValid(frame.pointer))
			return false;
		return bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	void UiButton::RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept
	{
		if (!enabled_)
		{
			visualPhase_ = UiVisualPhase::Disabled;
			return;
		}
		const bool focused = IsPointerOver(frame) || focus.IsFocused(focusHandle_);
		const bool pressVisual = (trackingPointerPress_ && frame.pointer.primaryDown)
			|| (trackingConfirmPress_ && frame.action.confirmDown);
		if (pressVisual)
			visualPhase_ = UiVisualPhase::Pressed;
		else if (focused)
			visualPhase_ = UiVisualPhase::Focused;
		else
			visualPhase_ = UiVisualPhase::Normal;
	}

	void UiButton::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			trackingConfirmPress_ = false;
			pointerWasDownLastFrame_ = frame.pointer.primaryDown;
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
		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_ && over)
			trackingPointerPress_ = true;
		if (!frame.pointer.primaryDown)
		{
			if (trackingPointerPress_)
			{
				trackingPointerPress_ = false;
				const bool releaseInside = ClientPointValid(frame.pointer)
					&& bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
				if (releaseInside && onClick_)
					onClick_();
			}
		}
		pointerWasDownLastFrame_ = frame.pointer.primaryDown;
		RecomputeVisualPhase(frame, focus);
	}

	void UiButton::ResetPointerInteraction() noexcept
	{
		trackingPointerPress_ = false;
		trackingConfirmPress_ = false;
		pointerWasDownLastFrame_ = false;
	}
}