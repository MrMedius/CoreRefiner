#include "UiStepper.h"

#include "FocusManager.h"
#include "InputCodex.h"
#include "Win.h"

#include <algorithm>

namespace Ui
{
	UiStepper::UiStepper(const FocusHandle focusHandle, const UiRect minusBounds, const UiRect plusBounds)
		:
		focusHandle_(focusHandle),
		minusBounds_(minusBounds),
		plusBounds_(plusBounds)
	{}

	void UiStepper::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			minusPress_.Reset();
			plusPress_.Reset();
			trackingMinusKey_ = false;
			trackingPlusKey_  = false;
		}
	}

	void UiStepper::SetRange(const float minValue, const float maxValue) noexcept
	{
		min_   = minValue;
		max_   = (maxValue > minValue) ? maxValue : minValue + 1.0f;
		value_ = std::clamp(value_, min_, max_);
	}

	void UiStepper::SetValue(const float value, const bool notify) noexcept
	{
		const float prev = value_;
		value_ = std::clamp(value, min_, max_);
		if (notify && onValueChanged_ && value_ != prev)
			onValueChanged_(value_);
	}

	bool UiStepper::IsPointerOverMinus_(const UiInputFrame& frame) const noexcept
	{
		if (!Input::ClientPointValid(frame.pointer))
			return false;
		return minusBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	bool UiStepper::IsPointerOverPlus_(const UiInputFrame& frame) const noexcept
	{
		if (!Input::ClientPointValid(frame.pointer))
			return false;
		return plusBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	void UiStepper::StepDown_() noexcept
	{
		SetValue(value_ - step_);
	}

	void UiStepper::StepUp_() noexcept
	{
		SetValue(value_ + step_);
	}

	void UiStepper::HandleKeyboard_(const FocusManager& focus) noexcept
	{
		const bool focused = focus.IsFocused(focusHandle_);
		if (!focused)
		{
			trackingMinusKey_ = false;
			trackingPlusKey_  = false;
			return;
		}

		const InputCodex& in = InputCodex::Get();

		if (in.KeyTriggered(VK_LEFT))
			trackingMinusKey_ = true;
		if (in.KeyTriggered(VK_RIGHT))
			trackingPlusKey_ = true;

		if (in.KeyReleased(VK_LEFT))
		{
			if (trackingMinusKey_)
			{
				trackingMinusKey_ = false;
				if (focus.IsFocused(focusHandle_))
					StepDown_();
			}
		}
		else if (trackingMinusKey_ && !in.KeyPressed(VK_LEFT))
			trackingMinusKey_ = false;

		if (in.KeyReleased(VK_RIGHT))
		{
			if (trackingPlusKey_)
			{
				trackingPlusKey_ = false;
				if (focus.IsFocused(focusHandle_))
					StepUp_();
			}
		}
		else if (trackingPlusKey_ && !in.KeyPressed(VK_RIGHT))
			trackingPlusKey_ = false;
	}

	void UiStepper::RecomputeVisualPhases_(const UiInputFrame& frame, const FocusManager& focus) noexcept
	{
		const InputCodex& in = InputCodex::Get();
		const bool minusPressVisual = minusPress_.ShouldShowPressed(frame.pointer.primaryDown)
			|| (trackingMinusKey_ && in.KeyPressed(VK_LEFT));
		const bool plusPressVisual = plusPress_.ShouldShowPressed(frame.pointer.primaryDown)
			|| (trackingPlusKey_ && in.KeyPressed(VK_RIGHT));

		minusPhase_ = ComputeStandardPhase(
			enabled_,
			minusPressVisual,
			IsPointerOverMinus_(frame),
			false);

		plusPhase_ = ComputeStandardPhase(
			enabled_,
			plusPressVisual,
			IsPointerOverPlus_(frame),
			false);

		showFocusRing_ = enabled_ && focus.IsFocused(focusHandle_);
	}

	void UiStepper::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		if (!enabled_)
		{
			minusPress_.Reset();
			plusPress_.Reset();
			minusPress_.SyncFrame(frame.pointer.primaryDown);
			plusPress_.SyncFrame(frame.pointer.primaryDown);
			RecomputeVisualPhases_(frame, focus);
			return;
		}

		HandleKeyboard_(focus);

		const bool overMinus = IsPointerOverMinus_(frame);
		const bool overPlus  = IsPointerOverPlus_(frame);

		minusPress_.TryBeginPress(frame.pointer.primaryDown, overMinus);
		plusPress_.TryBeginPress(frame.pointer.primaryDown, overPlus);

		const bool minusReleaseInside = Input::ClientPointValid(frame.pointer)
			&& minusBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
		if (minusPress_.TryCompletePress(frame.pointer.primaryDown, minusReleaseInside))
			StepDown_();

		const bool plusReleaseInside = Input::ClientPointValid(frame.pointer)
			&& plusBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
		if (plusPress_.TryCompletePress(frame.pointer.primaryDown, plusReleaseInside))
			StepUp_();

		minusPress_.SyncFrame(frame.pointer.primaryDown);
		plusPress_.SyncFrame(frame.pointer.primaryDown);

		RecomputeVisualPhases_(frame, focus);
	}

	void UiStepper::ResetPointerInteraction() noexcept
	{
		minusPress_.Reset();
		plusPress_.Reset();
		trackingMinusKey_ = false;
		trackingPlusKey_  = false;
	}
}
