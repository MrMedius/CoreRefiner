#pragma once
#include "FocusTypes.h"
#include "IUiLogic.h"
#include "UiPointerPressTracker.h"
#include "UiTypes.h"
#include "UiVisualPhase.h"

#include <functional>
#include <string>

namespace Ui
{
	class FocusManager;

	class UiStepper : public IUiLogic
	{
	public:
		UiStepper(FocusHandle focusHandle, UiRect minusBounds, UiRect plusBounds);

		void SetMinusBounds(UiRect r) noexcept { minusBounds_ = r; }
		void SetPlusBounds(UiRect r)  noexcept { plusBounds_  = r; }
		[[nodiscard]] const UiRect& GetMinusBounds() const noexcept { return minusBounds_; }
		[[nodiscard]] const UiRect& GetPlusBounds()  const noexcept { return plusBounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void SetRange(float minValue, float maxValue) noexcept;
		void SetValue(float value, bool notify = true) noexcept;
		void SetStep(float step) noexcept { step_ = step; }

		[[nodiscard]] float GetValue() const noexcept { return value_; }
		[[nodiscard]] float GetMin()   const noexcept { return min_; }
		[[nodiscard]] float GetMax()   const noexcept { return max_; }
		[[nodiscard]] float GetStep()  const noexcept { return step_; }

		void SetLabel(std::string utf8) { label_ = std::move(utf8); }
		[[nodiscard]] const std::string& GetLabel() const noexcept { return label_; }

		void SetOnValueChanged(std::function<void(float)> cb) { onValueChanged_ = std::move(cb); }

		[[nodiscard]] UiVisualPhase GetMinusPhase() const noexcept { return minusPhase_; }
		[[nodiscard]] UiVisualPhase GetPlusPhase()  const noexcept { return plusPhase_; }
		[[nodiscard]] bool GetShowFocusRing()       const noexcept { return showFocusRing_; }

		void Update(const UiInputFrame& frame, FocusManager& focus) override;
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }

	private:
		[[nodiscard]] bool IsPointerOverMinus_(const UiInputFrame& frame) const noexcept;
		[[nodiscard]] bool IsPointerOverPlus_(const UiInputFrame& frame)  const noexcept;
		void StepDown_() noexcept;
		void StepUp_()   noexcept;
		void HandleKeyboard_(const FocusManager& focus) noexcept;
		void RecomputeVisualPhases_(const UiInputFrame& frame, const FocusManager& focus) noexcept;

		FocusHandle focusHandle_;
		UiRect minusBounds_{};
		UiRect plusBounds_{};
		bool enabled_ = true;

		float min_   = 0.0f;
		float max_   = 10.0f;
		float value_ = 0.0f;
		float step_  = 1.0f;

		std::string label_;

		UiPointerPressTracker minusPress_;
		UiPointerPressTracker plusPress_;

		bool trackingMinusKey_ = false;
		bool trackingPlusKey_  = false;

		UiVisualPhase minusPhase_    = UiVisualPhase::Normal;
		UiVisualPhase plusPhase_     = UiVisualPhase::Normal;
		bool          showFocusRing_ = false;

		std::function<void(float)> onValueChanged_;
	};
}
