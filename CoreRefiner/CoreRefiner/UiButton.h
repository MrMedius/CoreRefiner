#pragma once

#include "FocusTypes.h"
#include "UiInputFrame.h"
#include "UiTypes.h"

#include <functional>
#include <string>


namespace Ui
{
	class FocusManager;

	enum class ButtonVisualPhase
	{
		Normal,		// not hovered, not focused, not pressed, enabled.
		Focused,	// focused by pointer (mouse) or keyboard or gamepad, but not pressed
		Pressed,	// pressed by pointer (mouse) or keyboard or gamepad
		Disabled	// not interactive
	};

	class UiButton
	{
	public:
		UiButton(FocusHandle focusHandle, UiRect bounds);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void SetLabel(std::string utf8) { labelUtf8_ = std::move(utf8); }
		[[nodiscard]] const std::string& GetLabel() const noexcept { return labelUtf8_; }

		void SetOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

		void Update(const UiInputFrame& frame, const FocusManager& focus);

		[[nodiscard]] ButtonVisualPhase GetVisualPhase() const noexcept { return visualPhase_; }

		void ResetPointerInteraction() noexcept;

	private:
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;

		FocusHandle focusHandle_;
		UiRect bounds_{};
		bool enabled_ = true;
		std::string labelUtf8_;

		bool trackingPointerPress_ = false;

		ButtonVisualPhase visualPhase_ = ButtonVisualPhase::Normal;

		std::function<void()> onClick_;

		bool pointerWasDownLastFrame_ = false;
	};
}