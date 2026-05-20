#include "UiSlider.h"

#include "FocusManager.h"
#include "Gamepad.h"
#include "InputCodex.h"
#include "Win.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	namespace
	{
		constexpr bool ClientPointValid(const UiPointerPayload& p) noexcept
		{
			return p.insideLogicalSurface;
		}
	}

	UiSlider::UiSlider(const FocusHandle focusHandle, UiRect bounds, const SliderAxis axis, const bool interactive)
		:
		focusHandle_(focusHandle),
		bounds_(bounds),
		axis_(axis),
		interactive_(interactive)
	{
		step_ = 0.01f;
	}

	void UiSlider::SetRange(const float minValue, const float maxValue) noexcept
	{
		min_ = minValue;
		max_ = (maxValue > minValue) ? maxValue : minValue + 1.0f;
		if (step_ <= 0.0f)
			step_ = (max_ - min_) / 100.0f;
		value_ = std::clamp(value_, min_, max_);
	}

	void UiSlider::SetValue(const float value) noexcept
	{
		const float prev = value_;
		value_ = std::clamp(value, min_, max_);
		if (onValueChanged_ && value_ != prev)
			onValueChanged_(value_);
	}

	void UiSlider::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = false;
		}
	}

	float UiSlider::GetNormalized() const noexcept
	{
		const float span = max_ - min_;
		if (span <= 0.0f)
			return 0.0f;
		return std::clamp((value_ - min_) / span, 0.0f, 1.0f);
	}

	bool UiSlider::IsFocusable() const noexcept
	{
		return interactive_ && enabled_ && focusHandle_ != kInvalidFocusHandle;
	}

	bool UiSlider::IsPointerOver(const UiInputFrame& frame) const noexcept
	{
		if (!ClientPointValid(frame.pointer))
			return false;
		return bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	float UiSlider::PointerToNormalized(const UiInputFrame& frame) const noexcept
	{
		const float spanW = bounds_.maxX - bounds_.minX;
		const float spanH = bounds_.maxY - bounds_.minY;
		if (axis_ == SliderAxis::Horizontal)
		{
			if (spanW <= 0.0f)
				return 0.0f;
			return std::clamp((frame.pointer.logicalX - bounds_.minX) / spanW, 0.0f, 1.0f);
		}
		if (spanH <= 0.0f)
			return 0.0f;
		return std::clamp((frame.pointer.logicalY - bounds_.minY) / spanH, 0.0f, 1.0f);
	}

	void UiSlider::ApplyNormalized(const float t) noexcept
	{
		const float clamped = std::clamp(t, 0.0f, 1.0f);
		SetValue(min_ + clamped * (max_ - min_));
	}

	void UiSlider::NudgeValue(const float delta) noexcept
	{
		if (delta == 0.0f)
			return;
		const float s = (step_ > 0.0f) ? step_ : (max_ - min_) / 100.0f;
		SetValue(value_ + delta * s);
	}

	void UiSlider::RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept
	{
		if (!interactive_ || !enabled_)
		{
			visualPhase_ = interactive_ ? UiVisualPhase::Disabled : UiVisualPhase::Normal;
			if (!enabled_)
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

	void UiSlider::HandleKeyboardGamepad_(const FocusManager& focus) noexcept
	{
		if (!interactive_ || !enabled_ || !focus.IsFocused(focusHandle_))
			return;

		const InputCodex& in = InputCodex::Get();
		bool decrease = false;
		bool increase = false;

		if (axis_ == SliderAxis::Horizontal)
		{
			decrease = in.KeyTriggered(VK_LEFT) || in.KeyTriggered(KK_A);
			increase = in.KeyTriggered(VK_RIGHT) || in.KeyTriggered(KK_D);
			if (in.PadConnected(gamepadIndex_))
			{
				decrease = decrease || in.GP_Triggered(gamepadIndex_, Gamepad::GP_DPAD_LEFT);
				increase = increase || in.GP_Triggered(gamepadIndex_, Gamepad::GP_DPAD_RIGHT);
			}
		}
		else
		{
			decrease = in.KeyTriggered(VK_UP) || in.KeyTriggered(KK_W);
			increase = in.KeyTriggered(VK_DOWN) || in.KeyTriggered(KK_S);
			if (in.PadConnected(gamepadIndex_))
			{
				decrease = decrease || in.GP_Triggered(gamepadIndex_, Gamepad::GP_DPAD_UP);
				increase = increase || in.GP_Triggered(gamepadIndex_, Gamepad::GP_DPAD_DOWN);
			}
		}

		if (decrease)
			NudgeValue(-1.0f);
		if (increase)
			NudgeValue(1.0f);
	}

	void UiSlider::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		if (!interactive_)
		{
			visualPhase_ = UiVisualPhase::Normal;
			return;
		}

		if (!enabled_)
		{
			trackingPointerPress_ = false;
			pointerWasDownLastFrame_ = frame.pointer.primaryDown;
			RecomputeVisualPhase(frame, focus);
			return;
		}

		HandleKeyboardGamepad_(focus);

		const bool over = IsPointerOver(frame);
		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_ && over)
			trackingPointerPress_ = true;

		if (trackingPointerPress_ && frame.pointer.primaryDown && over)
			ApplyNormalized(PointerToNormalized(frame));

		if (!frame.pointer.primaryDown)
			trackingPointerPress_ = false;

		pointerWasDownLastFrame_ = frame.pointer.primaryDown;
		RecomputeVisualPhase(frame, focus);
	}

	void UiSlider::ResetPointerInteraction() noexcept
	{
		trackingPointerPress_ = false;
		pointerWasDownLastFrame_ = false;
	}
}
