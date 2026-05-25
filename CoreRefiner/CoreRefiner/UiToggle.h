#pragma once
#include "FocusTypes.h"
#include "IUiLogic.h"
#include "UiTypes.h"
#include "UiVisualPhase.h"

#include <functional>

namespace Ui
{
	class FocusManager;

	/** @brief Unity 风格 Toggle 逻辑：bool isOn + 点击/确认键切换。 */
	class UiToggle : public IUiLogic
	{
	public:
		UiToggle(FocusHandle focusHandle, UiRect bounds);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		[[nodiscard]] bool IsOn() const noexcept { return isOn_; }
		void SetIsOn(bool on, bool notify = true) noexcept;
		[[nodiscard]] bool Toggle() noexcept;

		void SetOnValueChanged(std::function<void(bool)> cb) { onValueChanged_ = std::move(cb); }

		void Update(const UiInputFrame& frame, FocusManager& focus) override;
		[[nodiscard]] UiVisualPhase GetVisualPhase() const noexcept { return visualPhase_; }

		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }

	private:
		void ApplyIsOn_(bool on, bool notify) noexcept;
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;

		FocusHandle focusHandle_;
		UiRect bounds_{};
		bool enabled_ = true;
		bool isOn_ = false;

		bool trackingPointerPress_ = false;
		bool pointerWasDownLastFrame_ = false;
		UiVisualPhase visualPhase_ = UiVisualPhase::Normal;

		std::function<void(bool)> onValueChanged_;
	};
}
