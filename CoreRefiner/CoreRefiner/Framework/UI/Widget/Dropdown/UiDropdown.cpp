#include "UiDropdown.h"
#include "FocusManager.h"
#include "Graphics.h"
#include <algorithm>
namespace Ui
{

	UiDropdown::UiDropdown(const FocusHandle focusHandle, UiRect headerBounds)
		:
		focusHandle_(focusHandle),
		headerBounds_(headerBounds)
	{
		//options_ = {
		//	UiOption{ .label = "Low" },
		//	UiOption{ .label = "Medium" },
		//	UiOption{ .label = "High" }
		//};

		itemHeight_ = std::max(1.0f, headerBounds_.maxY - headerBounds_.minY);
	}

	void UiDropdown::SetHeaderBounds(UiRect r) noexcept
	{
		headerBounds_ = r;
		if (itemHeight_ <= 0.0f)
			itemHeight_ = std::max(1.0f, r.maxY - r.minY);
		RebuildItemBounds_();
	}

	void UiDropdown::SetItemHeight(const float logicalHeight) noexcept
	{
		itemHeight_ = std::max(1.0f, logicalHeight);
		RebuildItemBounds_();
	}

	float UiDropdown::GetListHeight() const noexcept
	{
		if (optionList_->GetOptions().empty())
			return 0.0f;
		return static_cast<float>(optionList_->GetOptions().size()) * itemHeight_;
	}

	void UiDropdown::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			headerPress_.Reset();
			listPress_.Reset();
			SetExpanded_(false);
		}
	}

	void UiDropdown::SetOptions(std::vector<UiOption> options)
	{
		optionList_->SetOptions(std::move(options));
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::AddOptions(std::vector<UiOption> options)
	{
		optionList_->AddOptions(std::move(options));
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::AddOption(const UiOption option)
	{
		optionList_->AddOption(option);
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::EraseOptions(std::vector<int> indices)
	{
		optionList_->EraseOptions(std::move(indices));
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::EraseOption(const int index)
	{
		EraseOptions(std::vector<int>{ index });
	}

	void UiDropdown::ClearOptions() noexcept
	{
		optionList_->ClearOptions();
		highlightIndex_ = -1;
		RebuildItemBounds_();
	}

	void UiDropdown::NormalizeSelectionAfterOptionsChange_() noexcept
	{
		const auto& options = optionList_->GetOptions();
		if (options.empty())
		{
			highlightIndex_ = -1;
			return;
		}

		if (highlightIndex_ >= static_cast<int>(options.size()))
			highlightIndex_ = static_cast<int>(options.size()) - 1;
	}

	void UiDropdown::SetSelectedIndex(const int index, const bool notify) noexcept
	{
		const int prevIndex = optionList_->GetSelectedIndex();
		optionList_->SetSelectedIndex(index, notify);
		if (optionList_->GetSelectedIndex() == prevIndex)
			return;
		if (notify && onValueChanged_)
			onValueChanged_(optionList_->GetSelectedIndex(), optionList_->GetSelectedLabel());
	}

	std::string UiDropdown::GetSelectedLabel() const
	{
		return optionList_->GetSelectedLabel();
	}

	void UiDropdown::RebuildItemBounds_() noexcept
	{
		itemBounds_.clear();
		listBounds_ = {};
		listOffsetY_ = 0.0f;
		const auto& options = optionList_->GetOptions();
		if (!expanded_ || options.empty() || itemHeight_ <= 0.0f)
			return;

		RecomputeListEdgeOffset_();
		const float left = headerBounds_.minX;
		const float right = headerBounds_.maxX;
		float y = headerBounds_.maxY + listOffsetY_;
		itemBounds_.reserve(options.size());
		for (size_t i = 0; i < options.size(); ++i)
		{
			const UiRect item{
				.minX = left,
				.minY = y,
				.maxX = right,
				.maxY = y + itemHeight_
			};

			itemBounds_.push_back(item);
			y += itemHeight_;
			if (i == 0)
				listBounds_ = item;
			else
			{
				listBounds_.minX = std::min(listBounds_.minX, item.minX);
				listBounds_.minY = std::min(listBounds_.minY, item.minY);
				listBounds_.maxX = std::max(listBounds_.maxX, item.maxX);
				listBounds_.maxY = std::max(listBounds_.maxY, item.maxY);
			}

		}

	}

	void UiDropdown::RecomputeListEdgeOffset_() noexcept
	{
		listOffsetY_ = 0.0f;
		const float listH = GetListHeight();
		const float listTop = headerBounds_.maxY;
		const float listBottom = listTop + listH;
		constexpr float screenH = static_cast<float>(LOGICAL_CANVAS_HEIGHT);
		if (listBottom > screenH)
			listOffsetY_ = screenH - listBottom;
		if (listTop + listOffsetY_ < 0.0f)
			listOffsetY_ -= listTop + listOffsetY_;
	}

	void UiDropdown::SetExpanded_(const bool expanded) noexcept
	{
		if (expanded_ == expanded)
			return;
		expanded_ = expanded;
		if (expanded_)
		{
			highlightIndex_ = -1;
			keyboardListNavPrimed_ = false;
			RebuildItemBounds_();
		}

		else
		{
			itemBounds_.clear();
			listBounds_ = {};
			highlightIndex_ = -1;
			keyboardListNavPrimed_ = false;
			listPress_.Reset();
			listItemPhases_.clear();
		}

	}

	void UiDropdown::ToggleExpanded_() noexcept
	{
		SetExpanded_(!expanded_);
	}

	void UiDropdown::ReleaseFocusIfHeld_(FocusManager& focus) noexcept
	{
		if (focus.IsFocused(focusHandle_))
			focus.ClearFocus();
	}

	void UiDropdown::Collapse_() noexcept
	{
		SetExpanded_(false);
	}

	void UiDropdown::CollapseFromPointer_(FocusManager& focus) noexcept
	{
		Collapse_();
		ReleaseFocusIfHeld_(focus);
	}

	bool UiDropdown::IsPointerOverHeader_(const UiInputFrame& frame) const noexcept
	{
		if (!Input::ClientPointValid(frame.pointer))
			return false;
		return headerBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	bool UiDropdown::IsPointerOverList_(const UiInputFrame& frame) const noexcept
	{
		if (!expanded_ || !Input::ClientPointValid(frame.pointer))
			return false;
		return HitTestItemIndex_(frame.pointer.logicalX, frame.pointer.logicalY) >= 0;
	}

	int UiDropdown::HitTestItemIndex_(const float x, const float y) const noexcept
	{
		if (!expanded_)
			return -1;
		for (int i = 0; i < static_cast<int>(itemBounds_.size()); ++i)
		{
			if (itemBounds_[static_cast<size_t>(i)].Contains(x, y))
				return i;
		}

		return -1;
	}

	bool UiDropdown::ClaimsPointerInteraction(const float x, const float y) const noexcept
	{
		if (headerBounds_.Contains(x, y))
			return true;
		if (!expanded_)
			return false;
		return HitTestItemIndex_(x, y) >= 0;
	}

	bool UiDropdown::BlocksUnderlyingPointerAt(const float x, const float y) const noexcept
	{
		if (headerBounds_.Contains(x, y))
			return true;
		if (!expanded_)
			return false;

		const float panelTop = headerBounds_.maxY + listOffsetY_;
		const float panelBottom = panelTop + GetListHeight();
		const UiRect expandedPanelBounds{
			.minX = headerBounds_.minX,
			.minY = panelTop,
			.maxX = headerBounds_.maxX,
			.maxY = panelBottom
		};
		return expandedPanelBounds.Contains(x, y);
	}

	bool UiDropdown::IsPointerInsideDropdown_(const float x, const float y) const noexcept
	{
		return BlocksUnderlyingPointerAt(x, y);
	}

	void UiDropdown::TryCollapseOnExternalInteraction_(
		const UiInputFrame& frame,
		FocusManager& focus) noexcept
	{
		if (expanded_ && (frame.navigation.tabNext || frame.navigation.tabPrev))
		{
			Collapse_();
			if (frame.navigation.tabNext)
				focus.FocusNext();
			else
				focus.FocusPrev();
			return;
		}

		if (!Input::ClientPointValid(frame.pointer))
			return;
		const float px = frame.pointer.logicalX;
		const float py = frame.pointer.logicalY;
		if (frame.pointer.primaryDown && !headerPress_.PrimaryDownLastFrame()
			&& !IsPointerInsideDropdown_(px, py))
		{
			if (expanded_)
				CollapseFromPointer_(focus);
			else
				ReleaseFocusIfHeld_(focus);
		}

	}

	void UiDropdown::UpdateHighlightFromPointer_(const UiInputFrame& frame) noexcept
	{
		if (!expanded_ || !Input::ClientPointValid(frame.pointer))
			return;
		keyboardListNavPrimed_ = false;
		if (IsPointerOverList_(frame))
			highlightIndex_ = HitTestItemIndex_(frame.pointer.logicalX, frame.pointer.logicalY);
		else
			highlightIndex_ = -1;
	}

	void UiDropdown::HandleExpandedListNavigation_(const UiInputFrame& frame) noexcept
	{
		if (!expanded_)
			return;
		if (!frame.navigation.navUp && !frame.navigation.navDown)
			return;
		if (!Input::ClientPointValid(frame.pointer) && !keyboardListNavPrimed_)
		{
			highlightIndex_ = -1;
			keyboardListNavPrimed_ = true;
		}

		if (frame.navigation.navDown)
			MoveListHighlight_(1);
		if (frame.navigation.navUp)
			MoveListHighlight_(-1);
	}

	void UiDropdown::MoveListHighlight_(const int delta) noexcept
	{
		const auto& options = optionList_->GetOptions();
		if (!expanded_ || options.empty() || delta == 0)
			return;
		const int lastIndex = static_cast<int>(options.size()) - 1;
		if (delta > 0)
		{
			if (highlightIndex_ < 0)
				highlightIndex_ = 0;
			else if (highlightIndex_ < lastIndex)
				++highlightIndex_;
		}

		else
		{
			if (highlightIndex_ <= 0)
				highlightIndex_ = -1;
			else
				--highlightIndex_;
		}

	}

	bool UiDropdown::ConsumesDirectionalNavigation() const noexcept
	{
		return enabled_ && expanded_;
	}

	bool UiDropdown::IsListItemPressed_(const int index, const UiInputFrame& frame) const noexcept
	{
		if (index < 0)
			return false;
		if (listPress_.ShouldShowPressed(index, frame.pointer.primaryDown))
			return true;
		if (frame.action.confirmDown && highlightIndex_ == index)
			return true;
		return false;
	}

	void UiDropdown::RecomputeListItemPhases_(const UiInputFrame& frame) noexcept
	{
		const auto& options = optionList_->GetOptions();
		listItemPhases_.assign(options.size(), UiVisualPhase::Normal);
		if (!expanded_)
			return;

		const int selectedIndex = optionList_->GetSelectedIndex();
		for (size_t i = 0; i < options.size(); ++i)
		{
			const int idx = static_cast<int>(i);
			if (IsListItemPressed_(idx, frame))
			{
				listItemPhases_[i] = UiVisualPhase::Pressed;
				continue;
			}
			if (highlightIndex_ == idx || selectedIndex == idx)
				listItemPhases_[i] = UiVisualPhase::Focused;
		}
	}

	UiVisualPhase UiDropdown::GetListItemVisualPhase(const int index) const noexcept
	{
		if (index < 0 || index >= static_cast<int>(listItemPhases_.size()))
			return UiVisualPhase::Normal;
		return listItemPhases_[static_cast<size_t>(index)];
	}

	void UiDropdown::RecomputeHeaderPhase_(
		const UiInputFrame& frame,
		FocusManager& focus) noexcept
	{
		if (!enabled_)
		{
			headerPhase_ = UiVisualPhase::Disabled;
			return;
		}

		if (expanded_)
		{
			headerPhase_ = focus.IsFocused(focusHandle_)
				? UiVisualPhase::Focused
				: UiVisualPhase::Normal;
			return;
		}

		const bool overHeader = IsPointerOverHeader_(frame);
		const bool focused = overHeader || focus.IsFocused(focusHandle_);
		const bool pressVisual = headerPress_.ShouldShowPressed(frame.pointer.primaryDown);
		if (pressVisual)
			headerPhase_ = UiVisualPhase::Pressed;
		else if (focused)
			headerPhase_ = UiVisualPhase::Focused;
		else
			headerPhase_ = UiVisualPhase::Normal;
	}

	void UiDropdown::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		if (!enabled_)
		{
			headerPress_.Reset();
			listPress_.Reset();
			headerPress_.SyncFrame(frame.pointer.primaryDown);
			listPress_.SyncFrame(frame.pointer.primaryDown);
			RecomputeListItemPhases_(frame);
			RecomputeHeaderPhase_(frame, focus);
			return;
		}

		TryCollapseOnExternalInteraction_(frame, focus);

		if (frame.action.confirmPressed)
		{
			if (!expanded_)
			{
				if (focus.IsFocused(focusHandle_))
					SetExpanded_(true);
			}
			else if (highlightIndex_ >= 0)
			{
				SetSelectedIndex(highlightIndex_, true);
				Collapse_();
			}
			else
				Collapse_();
		}

		HandleExpandedListNavigation_(frame);
		if (frame.action.cancelPressed && expanded_)
			Collapse_();

		const bool overHeader = IsPointerOverHeader_(frame);
		if (expanded_)
			UpdateHighlightFromPointer_(frame);

		if (frame.pointer.primaryDown && !headerPress_.PrimaryDownLastFrame() && Input::ClientPointValid(frame.pointer))
		{
			const float px = frame.pointer.logicalX;
			const float py = frame.pointer.logicalY;
			if (expanded_)
			{
				const int hitItem = HitTestItemIndex_(px, py);
				if (hitItem >= 0)
					listPress_.TryBeginPress(frame.pointer.primaryDown, hitItem);
				else
					headerPress_.TryBeginPress(frame.pointer.primaryDown, overHeader);
			}
			else
			{
				headerPress_.TryBeginPress(frame.pointer.primaryDown, overHeader);
			}
		}

		if (!frame.pointer.primaryDown)
		{
			if (Input::ClientPointValid(frame.pointer) && expanded_)
			{
				const float px = frame.pointer.logicalX;
				const float py = frame.pointer.logicalY;
				const int hitItem = HitTestItemIndex_(px, py);
				int completedIndex = -1;
				if (listPress_.TryCompletePress(
					frame.pointer.primaryDown,
					hitItem,
					true,
					completedIndex))
				{
					SetSelectedIndex(completedIndex, true);
					CollapseFromPointer_(focus);
				}
			}
			else
			{
				int completedIndex = -1;
				listPress_.TryCompletePress(
					frame.pointer.primaryDown,
					-1,
					false,
					completedIndex);
			}

			if (headerPress_.EndPress(frame.pointer.primaryDown)
				&& Input::ClientPointValid(frame.pointer))
			{
				const float px = frame.pointer.logicalX;
				const float py = frame.pointer.logicalY;
				if (expanded_)
				{
					if (IsPointerOverHeader_(frame))
						CollapseFromPointer_(focus);
					else if (!IsPointerInsideDropdown_(px, py))
						CollapseFromPointer_(focus);
				}
				else if (IsPointerOverHeader_(frame))
				{
					ToggleExpanded_();
					focus.RequestFocus(focusHandle_);
				}
			}
		}

		headerPress_.SyncFrame(frame.pointer.primaryDown);
		listPress_.SyncFrame(frame.pointer.primaryDown);
		RecomputeListItemPhases_(frame);
		RecomputeHeaderPhase_(frame, focus);
	}

	void UiDropdown::ResetPointerInteraction() noexcept
	{
		headerPress_.Reset();
		listPress_.Reset();
	}

}
