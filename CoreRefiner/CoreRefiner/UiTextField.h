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
#include <vector>

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

		// 把光标放到这个字节下标。超出正文就收到末尾。点击定位用。
		void SetCaretByteIndex(std::size_t byteIndex) noexcept;

		void SetPlaceholder(std::string utf8) { placeholderUtf8_ = std::move(utf8); }
		[[nodiscard]] const std::string& GetPlaceholder() const noexcept { return placeholderUtf8_; }

		void SetFontSize(float size) noexcept { fontSize_ = size; }
		[[nodiscard]] float GetFontSize() const noexcept { return fontSize_; }

		// 是否允许回车换行。关着时回车不插入换行，重命名和单行栏用这个。
		void SetMultiline(bool multiline) noexcept { multiline_ = multiline; }
		[[nodiscard]] bool IsMultiline() const noexcept { return multiline_; }

		// 超宽时是否按框宽另起一行显示。不往正文里写换行符。只在多行栏里生效。
		void SetAutoWrap(bool autoWrap) noexcept
		{
			autoWrap_ = autoWrap;
			if (!autoWrap_)
				visualLines_.clear();
		}
		[[nodiscard]] bool IsAutoWrap() const noexcept { return autoWrap_; }

		// 绘制层交回的显示行。自动换行的上下键和到顶到底都看它。
		void SetVisualLines(std::vector<EditLineSpan> lines) { visualLines_ = std::move(lines); }
		void ClearVisualLines() noexcept { visualLines_.clear(); }

		// 限制输入的最大字符数（按 UTF-8 codepoint 计）；0 表示不限制。
		// 若当前文本已超过新上限，会立即裁剪到上限并触发一次 OnTextChanged。
		void SetMaxLength(std::size_t maxCodepoints);
		[[nodiscard]] std::size_t GetMaxLength() const noexcept { return maxLength_; }

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
		// 上键是否还留在这一栏。多行、已聚焦、光标不在第一行时才留下。
		[[nodiscard]] bool ConsumesNavUp() const noexcept;
		// 下键是否还留在这一栏。多行、已聚焦、光标不在最后一行时才留下。
		[[nodiscard]] bool ConsumesNavDown() const noexcept;
		[[nodiscard]] bool ConsumesTextInput() const noexcept { return enabled_; }

	private:
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		[[nodiscard]] bool IsFocused(const FocusManager& focus) const noexcept;

		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;
		void ApplyTextInput(const UiTextInputPayload& text);
		// allowLineMove：这一帧之前已经聚焦时，上下键才改行。刚被选中的那一下不改行。
		void ApplyEditingKeys(const UiInputFrame& frame, bool allowLineMove);
		void NotifyTextChangedIfNeeded_();
		void ClampToMaxLength_();
		// 当前光标要不要按显示行判断。自动换行且显示行已经算好时用显示行。
		[[nodiscard]] bool UsesVisualLines_() const noexcept;
		[[nodiscard]] bool IsOnFirstLine_() const noexcept;
		[[nodiscard]] bool IsOnLastLine_() const noexcept;
		// 多行时往上或往下移一行。自动换行走显示行，否则走正文里的换行符。
		void MoveCaretLine_(bool up);

		FocusHandle focusHandle_;
		UiRect bounds_{};
		bool enabled_ = true;
		float fontSize_ = 18.0f;
		std::size_t maxLength_ = 0u;
		// 允许回车插入换行，并按多行排版。默认关，避免单行栏被回车撑开。
		bool multiline_ = false;
		// 超宽时按框宽折成显示行。默认关。正文里仍然只有回车才会出现换行符。
		bool autoWrap_ = false;
		// 最近一次排版给出的显示行。空的时候上下键退回按换行符分行。
		std::vector<EditLineSpan> visualLines_;

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
		// 左右键按住的帧数。超过等待后再按间隔移动光标。
		unsigned leftHoldFrames_ = 0u;
		unsigned rightHoldFrames_ = 0u;
		// 多行时上下键按住的帧数。用来在行与行之间移动光标。
		unsigned upHoldFrames_ = 0u;
		unsigned downHoldFrames_ = 0u;

		std::function<void(const std::string&)> onTextChanged_;
	};
}
