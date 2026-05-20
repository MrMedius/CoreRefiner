#include "UiRoot.h"

#include "IUiComponent.h"

namespace Ui
{
	void UiRoot::Clear() noexcept
	{
		components_.clear();
		focus_.ClearTabOrder();

		dominance_ = UiInputDominance::Mouse;
		pointerPrimaryWasDown_ = false;
		hasLastPointer_ = false;
	}

	void UiRoot::AddUiComponent(IUiComponent* component)
	{
		if (component == nullptr)
			return;
		components_.push_back(component);
	}

	void UiRoot::RebuildTabOrder()
	{
		std::vector<FocusHandle> order;
		order.reserve(components_.size());
		for (IUiComponent* c : components_)
		{
			if (c->IsFocusable() && c->GetFocusHandle() != kInvalidFocusHandle)
				order.push_back(c->GetFocusHandle());
		}
		focus_.SetTabOrder(std::move(order));
	}

	void UiRoot::InitLinkTechniques(Rgph::RenderGraph& rg)
	{
		for (IUiComponent* c : components_)
			c->LinkTechniques(rg);
	}

	void UiRoot::StripPointerForWidgets_(UiInputFrame& out) noexcept
	{
		out.pointer.logicalX = 0.0f;
		out.pointer.logicalY = 0.0f;
		out.pointer.insideLogicalSurface = false;
		out.pointer.primaryDown = false;
		out.pointer.primaryPressed = false;
		out.pointer.primaryReleased = false;
	}

	void UiRoot::UpdateAfterInput()
	{
		const UiInputFrame m = mouse_.BuildFrame(true);
		const UiInputFrame kbd = keyboard_.BuildFrame();
		const UiInputFrame pad = gamepad_.BuildFrame();

		const bool nonPointerSemantic =
			kbd.navigation.tabNext || kbd.navigation.tabPrev
			|| kbd.action.confirmPressed || kbd.action.cancelPressed
			|| pad.navigation.tabNext || pad.navigation.tabPrev
			|| pad.action.confirmPressed || pad.action.cancelPressed;

		const bool inside = m.pointer.insideLogicalSurface;
		bool movedEnough = false;
		if (inside && hasLastPointer_)
		{
			const float dx = m.pointer.logicalX - lastPointerLogicalX_;
			const float dy = m.pointer.logicalY - lastPointerLogicalY_;
			const float thr = kMouseMoveDominanceThresholdPx;
			movedEnough = (dx * dx + dy * dy > thr * thr);
		}
		const bool mouseDownEdge = inside && m.pointer.primaryDown && !pointerPrimaryWasDown_;

		const UiInputDominance prev = dominance_;
		if (dominance_ == UiInputDominance::Mouse && nonPointerSemantic)
			dominance_ = UiInputDominance::NonPointer;
		else if (dominance_ == UiInputDominance::NonPointer && (movedEnough || mouseDownEdge))
			dominance_ = UiInputDominance::Mouse;

		if (dominance_ != prev)
		{
			if (dominance_ == UiInputDominance::Mouse)
				ResetToMouseDominantState_();
			else
				ResetToNonPointerDominantState_();
		}

		if (inside)
		{
			lastPointerLogicalX_ = m.pointer.logicalX;
			lastPointerLogicalY_ = m.pointer.logicalY;
			hasLastPointer_ = true;
		}
		else
			hasLastPointer_ = false;

		pointerPrimaryWasDown_ = m.pointer.primaryDown;

		UiInputFrame frame = m;
		MergeUiInputFramesOr(frame, kbd);
		MergeUiInputFramesOr(frame, pad);

		UiInputFrame widgetFrame = frame;
		if (dominance_ == UiInputDominance::NonPointer)
			StripPointerForWidgets_(widgetFrame);

		focus_.ApplyNavigation(frame);
		for (IUiComponent* c : components_)
		{
			c->Update(widgetFrame, focus_);
			c->SyncView();
		}
	}

	void UiRoot::ResetToMouseDominantState_()
	{
		focus_.ClearFocus();
		for (IUiComponent* c : components_)
			c->ResetPointerInteraction();
	}

	void UiRoot::ResetToNonPointerDominantState_()
	{
		for (IUiComponent* c : components_)
			c->ResetPointerInteraction();
	}

	void UiRoot::Submit(const std::size_t channelMask) const
	{
		for (const IUiComponent* c : components_)
			c->Submit(channelMask);
	}
}