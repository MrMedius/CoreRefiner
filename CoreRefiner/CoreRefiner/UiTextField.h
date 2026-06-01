#pragma once

#include "FocusTypes.h"
#include "IUiLogic.h"
#include "UiInputFrame.h"
#include "UiPointerPressTracker.h"
#include "UiTypes.h"
#include "UiUtf8EditBuffer.h"
#include "UiVisualPhase.h"

#include <functional>
#include <string>

namespace Ui
{
	class FocusManager;

	class UiTextField : public IUiLogic
	{
	public:
		UiTextField(FocusHandle focusHandle, UiRect bounds);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void SetText(std::string utf8);
		[[nodiscard]] const std::string& GetText() const noexcept { return buffer_.GetText(); }

		void SetPlaceholder(std::string utf8) { placeholderUtf8_ = std::move(utf8); }
		[[nodiscard]] const std::string& GetPlaceholder() const noexcept { return placeholderUtf8_; }

		void SetFontSize(float size) noexcept { fontSize_ = size; }
		[[nodiscard]] float GetFontSize() const noexcept { return fontSize_; }

		void SetOnTextChanged(std::function<void(const std::string&)> cb) { onTextChanged_ = std::move(cb); }

		void Update(const UiInputFrame& frame, FocusManager& focus) override;

		[[nodiscard]] UiVisualPhase GetVisualPhase() const noexcept { return visualPhase_; }
		[[nodiscard]] std::size_t GetCaretByteIndex() const noexcept { return buffer_.GetCaretByteIndex(); }
		[[nodiscard]] bool ShowCaret() const noexcept { return showCaret_; }

		[[nodiscard]] const std::string& GetImeComposition() const noexcept { return imeCompositionUtf8_; }
		[[nodiscard]] bool ImeCompositionActive() const noexcept { return imeCompositionActive_; }

		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }
		[[nodiscard]] bool ConsumesDirectionalNavigation() const noexcept;
		[[nodiscard]] bool ConsumesTextInput() const noexcept { return enabled_; }

	private:
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		[[nodiscard]] bool IsFocused(const FocusManager& focus) const noexcept;

		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;
		void ApplyTextInput(const UiTextInputPayload& text);
		void ApplyEditingKeys(const UiInputFrame& frame);
		void NotifyTextChangedIfNeeded_();

		FocusHandle focusHandle_;
		UiRect bounds_{};
		bool enabled_ = true;
		float fontSize_ = 18.0f;

		UiUtf8EditBuffer buffer_;
		std::string placeholderUtf8_;
		std::string imeCompositionUtf8_;
		bool imeCompositionActive_ = false;

		UiPointerPressTracker pointerPress_;
		UiVisualPhase visualPhase_ = UiVisualPhase::Normal;

		unsigned blinkFrameCounter_ = 0u;
		bool showCaret_ = true;
		bool focused_ = false;

		static constexpr unsigned kBackspaceInitialDelay = 20u;
		static constexpr unsigned kBackspaceRepeatInterval = 3u;
		unsigned backspaceHoldFrames_ = 0u;
		unsigned deleteHoldFrames_ = 0u;

		std::function<void(const std::string&)> onTextChanged_;
	};
}
