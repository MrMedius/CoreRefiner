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
			visualPhase_ = ButtonVisualPhase::Disabled;
			return;
		}
		const bool over = IsPointerOver(frame);
		const bool focused = focus.IsFocused(focusHandle_);
		const bool pressVisual = trackingPointerPress_ && frame.pointer.primaryDown;
		if (pressVisual)
			visualPhase_ = ButtonVisualPhase::Pressed;
		else if (over)
			visualPhase_ = ButtonVisualPhase::Hovered;
		else if (focused)
			visualPhase_ = ButtonVisualPhase::Focused;
		else
			visualPhase_ = ButtonVisualPhase::Normal;
	}

	void UiButton::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = frame.pointer.primaryDown;
			RecomputeVisualPhase(frame, focus);
			return;
		}
		if (focus.IsFocused(focusHandle_) && frame.action.confirmPressed)
		{
			if (onClick_)
				onClick_();
		}
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
		pointerWasDownLastFrame_ = false;
	}
}