#pragma once

#include "FocusTypes.h"
#include "IUiLogic.h"
#include "SliderAxis.h"
#include "UiTypes.h"
#include "UiVisualPhase.h"

#include <functional>

namespace Ui
{
	class FocusManager;

	/**
	 * @brief Unity 式 Slider：可交互/只读，水平或垂直。
	 */
	class UiSlider : public IUiLogic
	{
	public:
		UiSlider(FocusHandle focusHandle, UiRect bounds, SliderAxis axis, bool interactive = true);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		void SetAxis(SliderAxis axis) noexcept { axis_ = axis; }
		[[nodiscard]] SliderAxis GetAxis() const noexcept { return axis_; }

		void SetInteractive(bool on) noexcept { interactive_ = on; }
		[[nodiscard]] bool IsInteractive() const noexcept { return interactive_; }

		void SetRange(float minValue, float maxValue) noexcept;
		void SetValue(float value) noexcept;
		void SetStep(float step) noexcept { step_ = step; }
		void SetEnabled(bool enabled) noexcept;
		void SetGamepadPlayerIndex(int idx) noexcept { gamepadIndex_ = idx; }

		[[nodiscard]] float GetValue() const noexcept { return value_; }
		[[nodiscard]] float GetNormalized() const noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }
		[[nodiscard]] UiVisualPhase GetVisualPhase() const noexcept { return visualPhase_; }

		void SetOnValueChanged(std::function<void(float)> cb) { onValueChanged_ = std::move(cb); }

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;

	private:
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		[[nodiscard]] float PointerToNormalized(const UiInputFrame& frame) const noexcept;
		void ApplyNormalized(float t) noexcept;
		void NudgeValue(float delta) noexcept;
		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;
		void HandleKeyboardGamepad_(const FocusManager& focus) noexcept;

		FocusHandle focusHandle_;
		UiRect bounds_{};
		SliderAxis axis_ = SliderAxis::Horizontal;
		bool interactive_ = true;
		bool enabled_ = true;

		float min_ = 0.0f;
		float max_ = 1.0f;
		float value_ = 0.0f;
		float step_ = 0.0f;

		bool trackingPointerPress_ = false;
		bool pointerWasDownLastFrame_ = false;
		UiVisualPhase visualPhase_ = UiVisualPhase::Normal;

		int gamepadIndex_ = 0;
		std::function<void(float)> onValueChanged_;
	};
}
