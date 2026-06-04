#include "UiTextField.h"

#include "FocusManager.h"
#include "InputCodex.h"
#include "Keyboard.h"

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
		// 不再拦截上下方向导航：聚焦时仍允许用上下键切换控件；
		// W/S 的输入冲突由 KeyboardUiInputAdapter 在文本捕获时屏蔽。
		return false;
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

			if (maxLength_ == 0u)
			{
				buffer_.InsertUtf8(commit);
				inserted = true;
				continue;
			}

			const std::size_t current = Utf8CodepointCount(buffer_.GetText());
			if (current >= maxLength_)
				break;

			const std::size_t room = maxLength_ - current;
			const std::size_t incoming = Utf8CodepointCount(commit);
			if (incoming <= room)
			{
				buffer_.InsertUtf8(commit);
				inserted = true;
			}
			else
			{
				// 截断到剩余可容纳的 codepoint 数
				std::size_t end = 0u;
				for (std::size_t n = 0u; n < room; ++n)
					end = Utf8Next(commit, end);
				buffer_.InsertUtf8(std::string_view(commit).substr(0, end));
				inserted = true;
				break;
			}
		}

		imeCompositionActive_ = text.imeCompositionActive;
		imeCompositionUtf8_ = text.imeCompositionUtf8;

		if (inserted)
			NotifyTextChangedIfNeeded_();
	}

	void UiTextField::ApplyEditingKeys(const UiInputFrame& frame)
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

		if (in.KeyTriggered(KK_ENTER))
			buffer_.InsertUtf8("\n");

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

		const bool focused = IsFocused(focus);
		focused_ = focused;
		if (focused)
		{
			ApplyTextInput(frame.text);
			ApplyEditingKeys(frame);

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
