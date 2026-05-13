#include "UiRoot.h"

#include "ButtonViewModel.h"
#include "IButtonView.h"
#include "InputCodex.h"
#include "UiButton.h"

#include "Win.h"

namespace Ui
{
	void UiRoot::Clear() noexcept
	{
		slots_.clear();
		focus_.ClearTabOrder();
	}

	void UiRoot::AddButtonSlot(UiButton* btn, IButtonView* view)
	{
		if (btn == nullptr || view == nullptr)
			return;
		slots_.push_back(UiButtonSlot{ btn, view });
	}

	void UiRoot::RebuildTabOrderFromSlots()
	{
		std::vector<FocusHandle> order;
		order.reserve(slots_.size());
		for (const UiButtonSlot& s : slots_)
			order.push_back(s.button->GetFocusHandle());
		focus_.SetTabOrder(std::move(order));
	}

	void UiRoot::InitLinkTechniques(Rgph::RenderGraph& rg)
	{
		for (const UiButtonSlot& s : slots_)
			s.view->LinkTechniques(rg);
	}

	UiInputFrame UiRoot::BuildMergedInputFrame_() const
	{
		UiInputFrame frame = mouse_.BuildFrame(true);

		InputCodex& in = InputCodex::Get();
		if (in.KeyTriggered(VK_TAB))
		{
			if (in.KeyPressed(VK_SHIFT))
				frame.navigation.tabPrev = true;
			else
				frame.navigation.tabNext = true;
		}
		if (in.KeyTriggered(VK_RETURN) || in.KeyTriggered(VK_SPACE))
			frame.action.confirmPressed = true;

		return frame;
	}

	void UiRoot::TickAfterInput()
	{
		UiInputFrame frame = BuildMergedInputFrame_();
		focus_.ApplyNavigation(frame);

		for (UiButtonSlot& s : slots_)
		{
			s.button->Update(frame, focus_);
			s.view->SyncFrom(MakeButtonViewModel(*s.button));
		}
	}

	void UiRoot::Submit(std::size_t channelMask) const
	{
		for (const UiButtonSlot& s : slots_)
			s.view->Submit(channelMask);
	}
}