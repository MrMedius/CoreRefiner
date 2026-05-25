#include "UiDropdown.h"



#include "FocusManager.h"
#include "Graphics.h"



#include <algorithm>

#include <cmath>



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

		options_ = {

			DropdownOption{ .label = "Low" },

			DropdownOption{ .label = "Medium" },

			DropdownOption{ .label = "High" }

		};

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



	void UiDropdown::SetMaxListVisibleItems(const unsigned count) noexcept

	{

		maxListVisibleItems_ = std::max(1u, count);

		ClampScrollOffset_();

		if (expanded_)

			RebuildItemBounds_();

	}



	void UiDropdown::SetScrollbarWidth(const float logicalWidth) noexcept

	{

		scrollbarWidth_ = std::max(4.0f, logicalWidth);

		if (expanded_)

			RebuildItemBounds_();

	}



	int UiDropdown::GetVisibleItemCount() const noexcept

	{

		if (options_.empty())

			return 0;



		const int capacity = GetVisibleItemCapacity_();

		return std::min(capacity, static_cast<int>(options_.size()) - scrollOffset_);

	}



	bool UiDropdown::GetShowScrollbar() const noexcept

	{

		return expanded_ && NeedsScroll_();

	}



	float UiDropdown::GetListViewportHeight() const noexcept

	{

		return static_cast<float>(GetVisibleItemCount()) * itemHeight_;

	}



	float UiDropdown::GetScrollThumbNormalizedPos() const noexcept

	{

		const int maxScroll = GetMaxScrollOffset_();

		if (maxScroll <= 0)

			return 0.0f;



		return static_cast<float>(scrollOffset_) / static_cast<float>(maxScroll);

	}



	float UiDropdown::GetScrollThumbNormalizedSize() const noexcept

	{

		if (options_.empty())

			return 1.0f;



		const int capacity = GetVisibleItemCapacity_();

		return std::min(

			1.0f,

			static_cast<float>(capacity) / static_cast<float>(options_.size()));

	}



	int UiDropdown::GetVisibleItemCapacity_() const noexcept

	{

		return static_cast<int>(std::max(1u, maxListVisibleItems_));

	}



	int UiDropdown::GetMaxScrollOffset_() const noexcept

	{

		if (!NeedsScroll_())

			return 0;



		return static_cast<int>(options_.size()) - GetVisibleItemCapacity_();

	}



	bool UiDropdown::NeedsScroll_() const noexcept

	{

		return static_cast<int>(options_.size()) > GetVisibleItemCapacity_();

	}



	void UiDropdown::ClampScrollOffset_() noexcept

	{

		scrollOffset_ = std::clamp(scrollOffset_, 0, GetMaxScrollOffset_());

	}



	void UiDropdown::EnsureHighlightVisible_() noexcept

	{

		if (highlightIndex_ < 0 || !NeedsScroll_())

			return;



		const int capacity = GetVisibleItemCapacity_();

		if (highlightIndex_ < scrollOffset_)

			scrollOffset_ = highlightIndex_;

		else if (highlightIndex_ >= scrollOffset_ + capacity)

			scrollOffset_ = highlightIndex_ - capacity + 1;



		ClampScrollOffset_();

	}



	void UiDropdown::SetEnabled(const bool enabled) noexcept

	{

		enabled_ = enabled;

		if (!enabled_)

		{

			trackingPointerPress_ = false;

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



		ClampScrollOffset_();

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

		scrollbarTrackBounds_ = {};

		scrollbarThumbBounds_ = {};

		listOffsetY_ = 0.0f;



		if (!expanded_ || options_.empty() || itemHeight_ <= 0.0f)

			return;



		ClampScrollOffset_();

		RecomputeListEdgeOffset_();



		const float left = headerBounds_.minX;

		const float right = headerBounds_.maxX;

		float y = headerBounds_.maxY + listOffsetY_;



		const int visibleCount = GetVisibleItemCount();

		itemBounds_.reserve(static_cast<size_t>(visibleCount));



		for (int localIndex = 0; localIndex < visibleCount; ++localIndex)

		{

			const UiRect item{

				.minX = left,

				.minY = y,

				.maxX = right,

				.maxY = y + itemHeight_

			};

			itemBounds_.push_back(item);

			y += itemHeight_;



			if (localIndex == 0)

				listBounds_ = item;

			else

			{

				listBounds_.minX = std::min(listBounds_.minX, item.minX);

				listBounds_.minY = std::min(listBounds_.minY, item.minY);

				listBounds_.maxX = std::max(listBounds_.maxX, item.maxX);

				listBounds_.maxY = std::max(listBounds_.maxY, item.maxY);

			}

		}



		RecomputeScrollbarBounds_();

	}



	void UiDropdown::RecomputeListEdgeOffset_() noexcept

	{

		listOffsetY_ = 0.0f;



		const float listH = GetListViewportHeight();

		const float listTop = headerBounds_.maxY;

		const float listBottom = listTop + listH;

		constexpr float screenH = static_cast<float>(LOGICAL_CANVAS_HEIGHT);



		if (listBottom > screenH)

			listOffsetY_ = screenH - listBottom;



		if (listTop + listOffsetY_ < 0.0f)

			listOffsetY_ -= listTop + listOffsetY_;

	}



	void UiDropdown::RecomputeScrollbarBounds_() noexcept

	{

		scrollbarTrackBounds_ = {};

		scrollbarThumbBounds_ = {};



		if (!GetShowScrollbar())

			return;



		const float trackTop = headerBounds_.maxY + listOffsetY_;

		const float trackBottom = trackTop + GetListViewportHeight();



		scrollbarTrackBounds_ = {

			.minX = headerBounds_.maxX - scrollbarWidth_,

			.minY = trackTop,

			.maxX = headerBounds_.maxX,

			.maxY = trackBottom

		};



		const float trackH = trackBottom - trackTop;

		const float thumbH = std::max(itemHeight_ * 0.35f, trackH * GetScrollThumbNormalizedSize());

		const float movable = std::max(0.0f, trackH - thumbH);

		const float thumbTop = trackTop + movable * GetScrollThumbNormalizedPos();



		scrollbarThumbBounds_ = {

			.minX = scrollbarTrackBounds_.minX,

			.minY = thumbTop,

			.maxX = scrollbarTrackBounds_.maxX,

			.maxY = thumbTop + thumbH

		};

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

			trackingScrollbarDrag_ = false;

			scrollbarHovered_ = false;

			scrollOffset_ = 0;

			if (selectedIndex_ >= 0 && NeedsScroll_())

			{

				const int capacity = GetVisibleItemCapacity_();

				if (selectedIndex_ >= capacity)

					scrollOffset_ = std::min(selectedIndex_ - capacity + 1, GetMaxScrollOffset_());

			}

			ClampScrollOffset_();

			RebuildItemBounds_();

		}

		else

		{

			itemBounds_.clear();

			listBounds_ = {};

			scrollbarTrackBounds_ = {};

			scrollbarThumbBounds_ = {};

			highlightIndex_ = -1;

			keyboardListNavPrimed_ = false;

			trackingScrollbarDrag_ = false;

			scrollbarHovered_ = false;

			scrollOffset_ = 0;

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



	void UiDropdown::Collapse_(FocusManager& focus) noexcept

	{

		SetExpanded_(false);

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

		return listBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);

	}



	int UiDropdown::HitTestItemIndex_(const float x, const float y) const noexcept

	{

		if (!expanded_)

			return -1;



		for (int i = 0; i < static_cast<int>(itemBounds_.size()); ++i)

		{

			if (itemBounds_[static_cast<size_t>(i)].Contains(x, y))

				return scrollOffset_ + i;

		}

		return -1;

	}



	bool UiDropdown::IsPointerInsideDropdown_(const float x, const float y) const noexcept

	{

		if (headerBounds_.Contains(x, y))

			return true;

		if (!expanded_)

			return false;



		if (listBounds_.Contains(x, y))

			return true;



		return scrollbarTrackBounds_.Contains(x, y);

	}



	bool UiDropdown::IsPointerOverScrollbar_(const UiInputFrame& frame) const noexcept

	{

		if (!GetShowScrollbar() || !ClientPointValid(frame.pointer))

			return false;



		return scrollbarTrackBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY);

	}



	void UiDropdown::TryCollapseOnExternalInteraction_(

		const UiInputFrame& frame,

		FocusManager& focus) noexcept

	{

		if (frame.navigation.tabNext || frame.navigation.tabPrev)

		{

			if (expanded_)

				Collapse_(focus);

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

				Collapse_(focus);

			else

				ReleaseFocusIfHeld_(focus);

		}

	}



	void UiDropdown::UpdateHighlightFromPointer_(const UiInputFrame& frame) noexcept

	{

		if (!expanded_ || !ClientPointValid(frame.pointer) || trackingScrollbarDrag_)

			return;



		keyboardListNavPrimed_ = false;



		if (IsPointerOverList_(frame))

			highlightIndex_ = HitTestItemIndex_(frame.pointer.logicalX, frame.pointer.logicalY);

		else if (IsPointerOverScrollbar_(frame))

			highlightIndex_ = -1;

		else

			highlightIndex_ = -1;

	}



	void UiDropdown::HandleListScrollInput_(const UiInputFrame& frame) noexcept

	{

		if (!expanded_ || !NeedsScroll_() || frame.scroll.wheelSteps == 0)

			return;



		scrollOffset_ -= frame.scroll.wheelSteps;

		ClampScrollOffset_();

		RebuildItemBounds_();

	}



	void UiDropdown::ApplyScrollFromPointerY_(const float pointerY) noexcept

	{

		if (!GetShowScrollbar())

			return;



		const float trackTop = scrollbarTrackBounds_.minY;

		const float trackH = scrollbarTrackBounds_.maxY - trackTop;

		const float thumbH = scrollbarThumbBounds_.maxY - scrollbarThumbBounds_.minY;

		const float movable = std::max(0.0f, trackH - thumbH);

		if (movable <= 0.0f)

		{

			scrollOffset_ = 0;

			ClampScrollOffset_();

			return;

		}



		const float relY = std::clamp(pointerY - trackTop - thumbH * 0.5f, 0.0f, movable);

		const float t = relY / movable;

		const int maxScroll = GetMaxScrollOffset_();

		scrollOffset_ = static_cast<int>(std::lround(t * static_cast<float>(maxScroll)));

		ClampScrollOffset_();

	}



	void UiDropdown::UpdateScrollbarInteraction_(const UiInputFrame& frame) noexcept

	{

		if (!expanded_)

		{

			trackingScrollbarDrag_ = false;

			scrollbarHovered_ = false;

			return;

		}



		scrollbarHovered_ = IsPointerOverScrollbar_(frame);



		if (trackingScrollbarDrag_)

		{

			if (!frame.pointer.primaryDown)

			{

				trackingScrollbarDrag_ = false;

			}

			else if (ClientPointValid(frame.pointer))

			{

				ApplyScrollFromPointerY_(frame.pointer.logicalY);

				RebuildItemBounds_();

			}

			return;

		}



		if (!GetShowScrollbar() || !ClientPointValid(frame.pointer))

			return;



		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_)

		{

			if (scrollbarTrackBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY))

			{

				trackingScrollbarDrag_ = true;

				keyboardListNavPrimed_ = false;



				if (!scrollbarThumbBounds_.Contains(frame.pointer.logicalX, frame.pointer.logicalY))

					ApplyScrollFromPointerY_(frame.pointer.logicalY);



				RebuildItemBounds_();

			}

		}

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



		EnsureHighlightVisible_();

		if (NeedsScroll_())

			RebuildItemBounds_();

	}



	bool UiDropdown::ConsumesDirectionalNavigation() const noexcept

	{

		return enabled_ && expanded_;

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

		const bool pressVisual = trackingPointerPress_ && frame.pointer.primaryDown && overHeader;

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

			trackingPointerPress_ = false;

			pointerWasDownLastFrame_ = frame.pointer.primaryDown;

			RecomputeHeaderPhase_(frame, focus);

			return;

		}



		TryCollapseOnExternalInteraction_(frame, focus);



		if (expanded_)

		{

			UpdateScrollbarInteraction_(frame);

			HandleListScrollInput_(frame);

		}



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

				Collapse_(focus);

			}

			else

				Collapse_(focus);

		}



		HandleExpandedListNavigation_(frame);



		if (frame.action.cancelPressed && expanded_)

			Collapse_(focus);



		const bool overHeader = IsPointerOverHeader_(frame);



		if (expanded_)

			UpdateHighlightFromPointer_(frame);



		if (frame.pointer.primaryDown && !pointerWasDownLastFrame_)

		{

			if (expanded_)

			{

				if (IsPointerOverList_(frame) || IsPointerOverScrollbar_(frame) || overHeader)

					trackingPointerPress_ = true;

			}

			else if (overHeader)

				trackingPointerPress_ = true;

		}



		if (!frame.pointer.primaryDown && trackingPointerPress_)

		{

			trackingPointerPress_ = false;



			if (ClientPointValid(frame.pointer))

			{

				const float px = frame.pointer.logicalX;

				const float py = frame.pointer.logicalY;



				if (expanded_)

				{

					const int hitItem = HitTestItemIndex_(px, py);

					if (hitItem >= 0)

					{

						SetSelectedIndex(hitItem, true);

						Collapse_(focus);

					}

					else if (overHeader)

						Collapse_(focus);

					else if (!IsPointerInsideDropdown_(px, py))

						Collapse_(focus);

				}

				else if (overHeader)

				{

					ToggleExpanded_();

					focus.RequestFocus(focusHandle_);

				}

			}

		}



		pointerWasDownLastFrame_ = frame.pointer.primaryDown;

		RecomputeHeaderPhase_(frame, focus);

	}



	void UiDropdown::ResetPointerInteraction() noexcept

	{

		trackingPointerPress_ = false;

		trackingScrollbarDrag_ = false;

		scrollbarHovered_ = false;

		pointerWasDownLastFrame_ = false;

	}

}

