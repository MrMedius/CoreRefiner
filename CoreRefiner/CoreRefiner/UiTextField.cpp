#include "UiTextField.h"

#include "FocusManager.h"
#include "InputCodex.h"
#include "Keyboard.h"

#include <algorithm>

namespace Ui
{
	UiTextField::UiTextField(const FocusHandle focusHandle, const UiRect bounds)
		:
		focusHandle_(focusHandle),
		bounds_(bounds)
	{}

	void UiTextField::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			pointerPress_.Reset();
			imeCompositionActive_ = false;
			imeCompositionUtf8_.clear();
		}
	}

	void UiTextField::SetText(std::string utf8)
	{
		buffer_.SetText(std::move(utf8));
		ClampToMaxLength_();
	}

	void UiTextField::SetCaretByteIndex(const std::size_t byteIndex) noexcept
	{
		buffer_.SetCaretByteIndex(byteIndex);
		showCaret_ = true;
		blinkFrameCounter_ = 0u;
	}

	void UiTextField::SetMaxLength(const std::size_t maxCodepoints)
	{
		maxLength_ = maxCodepoints;
		ClampToMaxLength_();
	}

	void UiTextField::ClampToMaxLength_()
	{
		if (maxLength_ == 0u)
			return;

		const std::string& current = buffer_.GetText();
		if (Utf8CodepointCount(current) <= maxLength_)
			return;

		std::size_t end = 0u;
		for (std::size_t n = 0u; n < maxLength_; ++n)
			end = Utf8Next(current, end);

		buffer_.SetText(current.substr(0, end));
		NotifyTextChangedIfNeeded_();
	}

	bool UiTextField::IsPointerOver(const UiInputFrame& frame) const noexcept
	{
		if (!Input::ClientPointValid(frame.pointer))
			return false;
		return bounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	bool UiTextField::IsFocused(const FocusManager& focus) const noexcept
	{
		return focus.IsFocused(focusHandle_);
	}

	bool UiTextField::ConsumesDirectionalNavigation() const noexcept
	{
		// 上下里还有一键要留在这一栏。到顶或到底的那一键不算。
		return ConsumesNavUp() || ConsumesNavDown();
	}

	bool UiTextField::ConsumesNavUp() const noexcept
	{
		return multiline_ && focused_ && !buffer_.IsCaretOnFirstLine();
	}

	bool UiTextField::ConsumesNavDown() const noexcept
	{
		return multiline_ && focused_ && !buffer_.IsCaretOnLastLine();
	}

	void UiTextField::RecomputeVisualPhase(
		const UiInputFrame& frame,
		const FocusManager& focus) noexcept
	{
		const bool pressVisual = pointerPress_.ShouldShowPressed(frame.pointer.primaryDown);
		visualPhase_ = ComputeStandardPhase(
			enabled_,
			pressVisual,
			IsPointerOver(frame),
			focus.IsFocused(focusHandle_));
	}

	void UiTextField::ApplyTextInput(const UiTextInputPayload& text)
	{
		bool inserted = false;
		for (const std::string& commit : text.commitUtf8)
		{
			if (commit.empty())
				continue;

			// 单行栏丢掉回车，粘贴进来的换行也不会把字拆开。
			std::string stripped;
			const std::string* source = &commit;
			if (!multiline_)
			{
				stripped = commit;
				stripped.erase(std::remove(stripped.begin(), stripped.end(), '\n'), stripped.end());
				stripped.erase(std::remove(stripped.begin(), stripped.end(), '\r'), stripped.end());
				if (stripped.empty())
					continue;
				source = &stripped;
			}
			const std::string& piece = *source;

			if (maxLength_ == 0u)
			{
				buffer_.InsertUtf8(piece);
				inserted = true;
				continue;
			}

			const std::size_t current = Utf8CodepointCount(buffer_.GetText());
			if (current >= maxLength_)
				break;

			const std::size_t room = maxLength_ - current;
			const std::size_t incoming = Utf8CodepointCount(piece);
			if (incoming <= room)
			{
				buffer_.InsertUtf8(piece);
				inserted = true;
			}
			else
			{
				// 截断到剩余可容纳的 codepoint 数
				std::size_t end = 0u;
				for (std::size_t n = 0u; n < room; ++n)
					end = Utf8Next(piece, end);
				buffer_.InsertUtf8(std::string_view(piece).substr(0, end));
				inserted = true;
				break;
			}
		}

		imeCompositionActive_ = text.imeCompositionActive;
		imeCompositionUtf8_ = text.imeCompositionUtf8;

		if (inserted)
			NotifyTextChangedIfNeeded_();
	}

	void UiTextField::ApplyEditingKeys(const UiInputFrame& frame, const bool allowLineMove)
	{
		const InputCodex& in = InputCodex::Get();

		const std::size_t beforeSize = buffer_.GetText().size();

		if (in.KeyTriggered(KK_BACK))
		{
			buffer_.DeleteBackward();
			backspaceHoldFrames_ = 0u;
		}
		else if (in.KeyPressed(KK_BACK))
		{
			++backspaceHoldFrames_;
			if (backspaceHoldFrames_ > kBackspaceInitialDelay
				&& backspaceHoldFrames_ % kBackspaceRepeatInterval == 0u)
				buffer_.DeleteBackward();
		}
		else
			backspaceHoldFrames_ = 0u;

		if (in.KeyTriggered(KK_DELETE))
		{
			buffer_.DeleteForward();
			deleteHoldFrames_ = 0u;
		}
		else if (in.KeyPressed(KK_DELETE))
		{
			++deleteHoldFrames_;
			if (deleteHoldFrames_ > kBackspaceInitialDelay
				&& deleteHoldFrames_ % kBackspaceRepeatInterval == 0u)
				buffer_.DeleteForward();
		}
		else
			deleteHoldFrames_ = 0u;

		// 只有多行栏才把回车写进正文。满字数时不再插入换行。
		if (multiline_ && in.KeyTriggered(KK_ENTER))
		{
			if (maxLength_ == 0u || Utf8CodepointCount(buffer_.GetText()) < maxLength_)
				buffer_.InsertUtf8("\n");
		}

		// 左右键一次移动一个码点。按住时和退格一样，先等再重复。
		const std::size_t caretBefore = buffer_.GetCaretByteIndex();
		if (in.KeyTriggered(KK_LEFT))
		{
			buffer_.MoveCaretLeft();
			leftHoldFrames_ = 0u;
		}
		else if (in.KeyPressed(KK_LEFT))
		{
			++leftHoldFrames_;
			if (leftHoldFrames_ > kBackspaceInitialDelay
				&& leftHoldFrames_ % kBackspaceRepeatInterval == 0u)
				buffer_.MoveCaretLeft();
		}
		else
			leftHoldFrames_ = 0u;

		if (in.KeyTriggered(KK_RIGHT))
		{
			buffer_.MoveCaretRight();
			rightHoldFrames_ = 0u;
		}
		else if (in.KeyPressed(KK_RIGHT))
		{
			++rightHoldFrames_;
			if (rightHoldFrames_ > kBackspaceInitialDelay
				&& rightHoldFrames_ % kBackspaceRepeatInterval == 0u)
				buffer_.MoveCaretRight();
		}
		else
			rightHoldFrames_ = 0u;

		// 多行时上下键按行移动光标。刚聚焦的那一帧不移动，避免从别的控件走进来时多走一行。
		if (multiline_ && allowLineMove)
		{
			if (in.KeyTriggered(KK_UP))
			{
				buffer_.MoveCaretUp();
				upHoldFrames_ = 0u;
			}
			else if (in.KeyPressed(KK_UP))
			{
				++upHoldFrames_;
				if (upHoldFrames_ > kBackspaceInitialDelay
					&& upHoldFrames_ % kBackspaceRepeatInterval == 0u)
					buffer_.MoveCaretUp();
			}
			else
				upHoldFrames_ = 0u;

			if (in.KeyTriggered(KK_DOWN))
			{
				buffer_.MoveCaretDown();
				downHoldFrames_ = 0u;
			}
			else if (in.KeyPressed(KK_DOWN))
			{
				++downHoldFrames_;
				if (downHoldFrames_ > kBackspaceInitialDelay
					&& downHoldFrames_ % kBackspaceRepeatInterval == 0u)
					buffer_.MoveCaretDown();
			}
			else
				downHoldFrames_ = 0u;
		}
		else
		{
			upHoldFrames_ = 0u;
			downHoldFrames_ = 0u;
		}

		if (buffer_.GetCaretByteIndex() != caretBefore)
		{
			showCaret_ = true;
			blinkFrameCounter_ = 0u;
		}

		if (buffer_.GetText().size() != beforeSize)
			NotifyTextChangedIfNeeded_();

		(void)frame;
	}

	void UiTextField::NotifyTextChangedIfNeeded_()
	{
		if (onTextChanged_)
			onTextChanged_(buffer_.GetText());
	}

	void UiTextField::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		if (!enabled_)
		{
			pointerPress_.Reset();
			pointerPress_.SyncFrame(frame.pointer.primaryDown);
			RecomputeVisualPhase(frame, focus);
			return;
		}

		const bool over = IsPointerOver(frame);
		if (frame.pointer.primaryPressed && over)
			focus.RequestFocus(focusHandle_);
		else if (frame.pointer.primaryPressed && !over && IsFocused(focus))
			focus.ClearFocus();

		pointerPress_.TryBeginPress(frame.pointer.primaryDown, over);
		pointerPress_.SyncFrame(frame.pointer.primaryDown);

		const bool wasFocused = focused_;
		focused_ = IsFocused(focus);
		if (focused_)
		{
			ApplyTextInput(frame.text);
			ApplyEditingKeys(frame, wasFocused);

			++blinkFrameCounter_;
			if (blinkFrameCounter_ % 30u == 0u)
				showCaret_ = !showCaret_;
		}
		else
		{
			showCaret_ = true;
			blinkFrameCounter_ = 0u;
		}

		RecomputeVisualPhase(frame, focus);
	}

	void UiTextField::ResetPointerInteraction() noexcept
	{
		pointerPress_.Reset();
	}
}
