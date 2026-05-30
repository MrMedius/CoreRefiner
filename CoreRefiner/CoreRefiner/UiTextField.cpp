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
		return enabled_ && focused_;
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
		for (const std::string& commit : text.commitUtf8)
			buffer_.InsertUtf8(commit);

		imeCompositionActive_ = text.imeCompositionActive;
		imeCompositionUtf8_ = text.imeCompositionUtf8;

		if (!text.commitUtf8.empty())
			NotifyTextChangedIfNeeded_();
	}

	void UiTextField::ApplyEditingKeys(const UiInputFrame& frame)
	{
		const InputCodex& in = InputCodex::Get();

		if (in.KeyTriggered(KK_BACK))
			buffer_.DeleteBackward();
		if (in.KeyTriggered(KK_DELETE))
			buffer_.DeleteForward();

		if (in.KeyTriggered(KK_LEFT))
			buffer_.MoveCaretLeft();
		if (in.KeyTriggered(KK_RIGHT))
			buffer_.MoveCaretRight();
		if (in.KeyTriggered(KK_UP))
			buffer_.MoveCaretUp();
		if (in.KeyTriggered(KK_DOWN))
			buffer_.MoveCaretDown();
		if (in.KeyTriggered(KK_HOME))
			buffer_.MoveCaretLineHome();
		if (in.KeyTriggered(KK_END))
			buffer_.MoveCaretLineEnd();

		if (in.KeyTriggered(KK_ENTER))
			buffer_.InsertUtf8("\n");

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
