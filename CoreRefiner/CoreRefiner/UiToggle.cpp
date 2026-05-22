#include "UiToggle.h"
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

	UiToggle::UiToggle(const FocusHandle focusHandle, UiRect bounds)
		:
		focusHandle_(focusHandle),
		bounds_(bounds)
	{
	}

	void UiToggle::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = false;
		}
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
		if (!ClientPointValid(frame.pointer))
			return false;
		return bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	void UiToggle::RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept
	{
		if (!enabled_)
		{
			visualPhase_ = UiVisualPhase::Disabled;
			return;
		}

		const bool focused = IsPointerOver(frame) || focus.IsFocused(focusHandle_);
		const bool pressVisual = trackingPointerPress_ && frame.pointer.primaryDown;
		if (pressVisual)
			visualPhase_ = UiVisualPhase::Pressed;
		else if (focused)
			visualPhase_ = UiVisualPhase::Focused;
		else
			visualPhase_ = UiVisualPhase::Normal;
	}

	void UiToggle::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = frame.pointer.primaryDown;
			RecomputeVisualPhase(frame, focus);
			return;
		}

		if (focus.IsFocused(focusHandle_) && frame.action.confirmPressed)
			Toggle();

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
				if (releaseInside)
					Toggle();
			}
		}

		pointerWasDownLastFrame_ = frame.pointer.primaryDown;
		RecomputeVisualPhase(frame, focus);
	}

	void UiToggle::ResetPointerInteraction() noexcept
	{
		trackingPointerPress_ = false;
		pointerWasDownLastFrame_ = false;
	}
}
