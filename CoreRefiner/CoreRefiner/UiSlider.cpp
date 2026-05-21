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

		[[nodiscard]] float KeyAxisLocalX(const float rotationRadZ) noexcept
		{
			const InputCodex& in = InputCodex::Get();
			float worldX = 0.0f;
			float worldY = 0.0f;

			if (in.KeyTriggered(VK_LEFT) || in.KeyTriggered(KK_A))
				worldX -= 1.0f;
			if (in.KeyTriggered(VK_RIGHT) || in.KeyTriggered(KK_D))
				worldX += 1.0f;
			if (in.KeyTriggered(VK_UP) || in.KeyTriggered(KK_W))
				worldY -= 1.0f;
			if (in.KeyTriggered(VK_DOWN) || in.KeyTriggered(KK_S))
				worldY += 1.0f;

			const float c = std::cos(-rotationRadZ);
			const float s = std::sin(-rotationRadZ);
			return worldX * c - worldY * s;
		}

		[[nodiscard]] float GamepadAxisLocalX(const int gamepadIndex, const float rotationRadZ) noexcept
		{
			if (!InputCodex::Get().PadConnected(gamepadIndex))
				return 0.0f;

			const InputCodex& in = InputCodex::Get();
			float worldX = 0.0f;
			float worldY = 0.0f;

			if (in.GP_Triggered(gamepadIndex, Gamepad::GP_DPAD_LEFT))
				worldX -= 1.0f;
			if (in.GP_Triggered(gamepadIndex, Gamepad::GP_DPAD_RIGHT))
				worldX += 1.0f;
			if (in.GP_Triggered(gamepadIndex, Gamepad::GP_DPAD_UP))
				worldY -= 1.0f;
			if (in.GP_Triggered(gamepadIndex, Gamepad::GP_DPAD_DOWN))
				worldY += 1.0f;

			const float c = std::cos(-rotationRadZ);
			const float s = std::sin(-rotationRadZ);
			return worldX * c - worldY * s;
		}
	}

	UiSlider::UiSlider(const FocusHandle focusHandle, const bool interactive)
		:
		focusHandle_(focusHandle),
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

	bool UiSlider::HasValidGroove_() const noexcept
	{
		return grooveLayout_.grooveWidth > 0.0f && grooveLayout_.grooveHeight > 0.0f;
	}

	void UiSlider::PointerToLocal_(const float worldX, const float worldY, float& localX, float& localY) const noexcept
	{
		const float dx = worldX - grooveLayout_.centerX;
		const float dy = worldY - grooveLayout_.centerY;
		const float c = std::cos(-grooveLayout_.rotationRadZ);
		const float s = std::sin(-grooveLayout_.rotationRadZ);
		localX = dx * c - dy * s;
		localY = dx * s + dy * c;
	}

	bool UiSlider::IsPointerOver(const UiInputFrame& frame) const noexcept
	{
		if (!ClientPointValid(frame.pointer) || !HasValidGroove_())
			return false;

		float localX = 0.0f;
		float localY = 0.0f;
		PointerToLocal_(frame.pointer.logicalX, frame.pointer.logicalY, localX, localY);

		const float halfW = grooveLayout_.grooveWidth * 0.5f;
		const float halfH = grooveLayout_.grooveHeight * 0.5f;
		return std::abs(localX) <= halfW && std::abs(localY) <= halfH;
	}

	float UiSlider::PointerToNormalized(const UiInputFrame& frame) const noexcept
	{
		if (!HasValidGroove_())
			return 0.0f;

		float localX = 0.0f;
		float localY = 0.0f;
		PointerToLocal_(frame.pointer.logicalX, frame.pointer.logicalY, localX, localY);

		return std::clamp(localX / grooveLayout_.grooveWidth + 0.5f, 0.0f, 1.0f);
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

		const float localAxis = KeyAxisLocalX(grooveLayout_.rotationRadZ)
			+ GamepadAxisLocalX(gamepadIndex_, grooveLayout_.rotationRadZ);

		if (localAxis < -0.5f)
			NudgeValue(-1.0f);
		if (localAxis > 0.5f)
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

		if (trackingPointerPress_ && frame.pointer.primaryDown && ClientPointValid(frame.pointer))
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