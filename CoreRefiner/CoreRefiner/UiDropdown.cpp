#include "UiDropdown.h"
#include "FocusManager.h"
#include "Graphics.h"
#include <algorithm>
namespace Ui
{
	namespace
	{
		[[nodiscard]] bool ClientPointValid(const UiPointerPayload& p) noexcept
		{
			return p.insideLogicalSurface;
		}

	}

	UiDropdown::UiDropdown(const FocusHandle focusHandle, UiRect headerBounds)
		:
		focusHandle_(focusHandle),
		headerBounds_(headerBounds)
	{
		//options_ = {
		//	DropdownOption{ .label = "Low" },
		//	DropdownOption{ .label = "Medium" },
		//	DropdownOption{ .label = "High" }
		//};

		selectedIndex_ = 0;
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
		if (options_.empty())
			return 0.0f;
		return static_cast<float>(options_.size()) * itemHeight_;
	}

	void UiDropdown::SetEnabled(const bool enabled) noexcept
	{
		enabled_ = enabled;
		if (!enabled_)
		{
			trackingHeaderPress_ = false;
			listPressItemIndex_ = -1;
			pointerWasDownLastFrame_ = false;
			SetExpanded_(false);
		}

	}

	void UiDropdown::SetOptions(std::vector<DropdownOption> options)
	{
		options_ = std::move(options);
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::AddOptions(std::vector<DropdownOption> options)
	{
		if (options.empty())
			return;
		options_.insert(options_.end(),
			std::make_move_iterator(options.begin()),
			std::make_move_iterator(options.end()));
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::AddOption(const DropdownOption option)
	{
		options_.push_back(option);
		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::EraseOptions(std::vector<int> indices)
	{
		if (indices.empty() || options_.empty())
			return;
		std::sort(indices.begin(), indices.end());
		indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
		for (auto it = indices.rbegin(); it != indices.rend(); ++it)
		{
			const int idx = *it;
			if (idx < 0 || idx >= static_cast<int>(options_.size()))
				continue;
			options_.erase(options_.begin() + idx);
		}

		NormalizeSelectionAfterOptionsChange_();
		RebuildItemBounds_();
	}

	void UiDropdown::EraseOption(const int index)
	{
		EraseOptions(std::vector<int>{ index });
	}

	void UiDropdown::ClearOptions() noexcept
	{
		options_.clear();
		selectedIndex_ = -1;
		highlightIndex_ = -1;
		RebuildItemBounds_();
	}

	void UiDropdown::NormalizeSelectionAfterOptionsChange_() noexcept
	{
		if (options_.empty())
		{
			selectedIndex_ = -1;
			highlightIndex_ = -1;
			return;
		}

		if (selectedIndex_ < 0)
			selectedIndex_ = 0;
		else
			selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(options_.size()) - 1);
		if (highlightIndex_ >= static_cast<int>(options_.size()))
			highlightIndex_ = static_cast<int>(options_.size()) - 1;
	}

	void UiDropdown::SetSelectedIndex(const int index, const bool notify) noexcept
	{
		if (options_.empty())
		{
			selectedIndex_ = -1;
			return;
		}

		const int clamped = std::clamp(index, 0, static_cast<int>(options_.size()) - 1);
		if (selectedIndex_ == clamped)
			return;
		selectedIndex_ = clamped;
		if (notify && onValueChanged_)
			onValueChanged_(selectedIndex_, options_[static_cast<size_t>(selectedIndex_)].label);
	}

	std::string UiDropdown::GetSelectedLabel() const
	{
		if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(options_.size()))
			return {};
		return options_[static_cast<size_t>(selectedIndex_)].label;
	}

	void UiDropdown::RebuildItemBounds_() noexcept
	{
		itemBounds_.clear();
		listBounds_ = {};
		listOffsetY_ = 0.0f;
		if (!expanded_ || options_.empty() || itemHeight_ <= 0.0f)
			return;

		RecomputeListEdgeOffset_();
		const float left = headerBounds_.minX;
		const float right = headerBounds_.maxX;
		float y = headerBounds_.maxY + listOffsetY_;
		itemBounds_.reserve(options_.size());
		for (size_t i = 0; i < options_.size(); ++i)
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
			listPressItemIndex_ = -1;
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
		if (!ClientPointValid(frame.pointer))
			return false;
		return headerBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);
	}

	bool UiDropdown::IsPointerOverList_(const UiInputFrame& frame) const noexcept
	{
		if (!expanded_ || !ClientPointValid(frame.pointer))
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

		if (!ClientPointValid(frame.pointer))
			return;
		const float px = frame.pointer.logicalX;
		const float py = frame.pointer.logicalY;
		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_
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
		if (!expanded_ || !ClientPointValid(frame.pointer))
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
		if (!ClientPointValid(frame.pointer) && !keyboardListNavPrimed_)
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
		if (!expanded_ || options_.empty() || delta == 0)
			return;
		const int lastIndex = static_cast<int>(options_.size()) - 1;
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
		if (listPressItemIndex_ == index && frame.pointer.primaryDown)
			return true;
		if (frame.action.confirmDown && highlightIndex_ == index)
			return true;
		return false;
	}

	void UiDropdown::RecomputeListItemPhases_(const UiInputFrame& frame) noexcept
	{
		listItemPhases_.assign(options_.size(), UiVisualPhase::Normal);
		if (!expanded_)
			return;

		for (size_t i = 0; i < options_.size(); ++i)
		{
			const int idx = static_cast<int>(i);
			if (IsListItemPressed_(idx, frame))
			{
				listItemPhases_[i] = UiVisualPhase::Pressed;
				continue;
			}
			if (highlightIndex_ == idx || selectedIndex_ == idx)
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
		const bool pressVisual = trackingHeaderPress_ && frame.pointer.primaryDown;
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
			trackingHeaderPress_ = false;
			listPressItemIndex_ = -1;
			pointerWasDownLastFrame_ = frame.pointer.primaryDown;
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

		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_ && ClientPointValid(frame.pointer))
		{
			const float px = frame.pointer.logicalX;
			const float py = frame.pointer.logicalY;
			if (expanded_)
			{
				const int hitItem = HitTestItemIndex_(px, py);
				if (hitItem >= 0)
					listPressItemIndex_ = hitItem;
				else if (overHeader)
					trackingHeaderPress_ = true;
			}
			else if (overHeader)
			{
				trackingHeaderPress_ = true;
			}
		}

		if (!frame.pointer.primaryDown)
		{
			if (listPressItemIndex_ >= 0)
			{
				if (ClientPointValid(frame.pointer))
				{
					const float px = frame.pointer.logicalX;
					const float py = frame.pointer.logicalY;
					if (expanded_)
					{
						const int hitItem = HitTestItemIndex_(px, py);
						if (hitItem >= 0 && hitItem == listPressItemIndex_)
						{
							SetSelectedIndex(hitItem, true);
							CollapseFromPointer_(focus);
						}
					}
				}
				listPressItemIndex_ = -1;
			}

			if (trackingHeaderPress_)
			{
				trackingHeaderPress_ = false;
				if (ClientPointValid(frame.pointer))
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
		}

		pointerWasDownLastFrame_ = frame.pointer.primaryDown;
		RecomputeListItemPhases_(frame);
		RecomputeHeaderPhase_(frame, focus);
	}

	void UiDropdown::ResetPointerInteraction() noexcept
	{
		trackingHeaderPress_ = false;
		listPressItemIndex_ = -1;
		pointerWasDownLastFrame_ = false;
	}

}
