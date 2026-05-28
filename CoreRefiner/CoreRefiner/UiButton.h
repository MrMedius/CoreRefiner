#pragma once
#include "FocusTypes.h"
#include "IUiLogic.h"
#include "UiInputFrame.h"
#include "UiPointerPressTracker.h"
#include "UiTypes.h"
#include "UiVisualPhase.h"

#include <functional>
#include <string>


namespace Ui
{
	class FocusManager;

	class UiButton : public IUiLogic
	{
	public:
		UiButton(FocusHandle focusHandle, UiRect bounds);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void SetLabel(std::string utf8) { labelUtf8_ = std::move(utf8); }
		[[nodiscard]] const std::string& GetLabel() const noexcept { return labelUtf8_; }

		void SetOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

		void Update(const UiInputFrame& frame, FocusManager& focus) override;

		[[nodiscard]] UiVisualPhase GetVisualPhase() const noexcept { return visualPhase_; }

		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }

	private:
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;

		FocusHandle focusHandle_;
		UiRect bounds_{};
		bool enabled_ = true;
		std::string labelUtf8_;

		UiPointerPressTracker pointerPress_;
		bool trackingConfirmPress_ = false;

		UiVisualPhase visualPhase_ = UiVisualPhase::Normal;

		std::function<void()> onClick_;
	};
}