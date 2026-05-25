#pragma once

#include "FocusTypes.h"

#include "IUiLogic.h"

#include "UiTypes.h"

#include "UiVisualPhase.h"



#include <functional>

#include <string>

#include <vector>



namespace Ui

{

	class FocusManager;



	/** @brief Dropdown 单个选项（逻辑层数据，与渲染后端无关）。 */

	struct DropdownOption

	{

		std::string label;

	};



	/** @brief Dropdown 逻辑：展开/折叠、鼠标点选、列表 hit-test、滚动。 */

	class UiDropdown : public IUiLogic

	{

	public:

		UiDropdown(FocusHandle focusHandle, UiRect headerBounds);



		void SetHeaderBounds(UiRect r) noexcept;

		[[nodiscard]] const UiRect& GetHeaderBounds() const noexcept { return headerBounds_; }



		void SetItemHeight(float logicalHeight) noexcept;

		[[nodiscard]] float GetItemHeight() const noexcept { return itemHeight_; }



		void SetMaxListVisibleItems(unsigned count) noexcept;

		[[nodiscard]] unsigned GetMaxListVisibleItems() const noexcept { return maxListVisibleItems_; }

		void SetScrollbarWidth(float logicalWidth) noexcept;



		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }



		void SetEnabled(bool enabled) noexcept;

		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }



		void SetOptions(std::vector<DropdownOption> options);

		void AddOptions(std::vector<DropdownOption> options);

		void AddOption(DropdownOption option);

		void EraseOptions(std::vector<int> indices);

		void EraseOption(int index);

		void ClearOptions() noexcept;



		[[nodiscard]] const std::vector<DropdownOption>& GetOptions() const noexcept { return options_; }



		[[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex_; }

		void SetSelectedIndex(int index, bool notify = true) noexcept;



		[[nodiscard]] int GetHighlightIndex() const noexcept { return highlightIndex_; }

		[[nodiscard]] std::string GetSelectedLabel() const;



		[[nodiscard]] bool IsExpanded() const noexcept { return expanded_; }

		[[nodiscard]] bool ConsumesDirectionalNavigation() const noexcept;



		[[nodiscard]] float GetListOffsetY() const noexcept { return listOffsetY_; }

		[[nodiscard]] int GetScrollOffset() const noexcept { return scrollOffset_; }

		[[nodiscard]] int GetVisibleItemCount() const noexcept;

		[[nodiscard]] bool GetShowScrollbar() const noexcept;

		[[nodiscard]] float GetListViewportHeight() const noexcept;

		[[nodiscard]] float GetScrollbarWidth() const noexcept { return scrollbarWidth_; }

		[[nodiscard]] float GetScrollThumbNormalizedPos() const noexcept;

		[[nodiscard]] float GetScrollThumbNormalizedSize() const noexcept;

		[[nodiscard]] bool IsScrollbarHovered() const noexcept { return scrollbarHovered_; }



		void SetOnValueChanged(std::function<void(int index, const std::string& label)> cb)

		{

			onValueChanged_ = std::move(cb);

		}



		void Update(const UiInputFrame& frame, FocusManager& focus) override;

		[[nodiscard]] UiVisualPhase GetHeaderVisualPhase() const noexcept { return headerPhase_; }

		void ResetPointerInteraction() noexcept override;

		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }



	private:

		[[nodiscard]] int GetVisibleItemCapacity_() const noexcept;

		[[nodiscard]] int GetMaxScrollOffset_() const noexcept;

		[[nodiscard]] bool NeedsScroll_() const noexcept;



		void RebuildItemBounds_() noexcept;

		void RecomputeListEdgeOffset_() noexcept;

		void RecomputeScrollbarBounds_() noexcept;

		void NormalizeSelectionAfterOptionsChange_() noexcept;

		void ClampScrollOffset_() noexcept;

		void EnsureHighlightVisible_() noexcept;



		void RecomputeHeaderPhase_(const UiInputFrame& frame, FocusManager& focus) noexcept;



		[[nodiscard]] bool IsPointerOverHeader_(const UiInputFrame& frame) const noexcept;

		[[nodiscard]] bool IsPointerOverList_(const UiInputFrame& frame) const noexcept;

		[[nodiscard]] bool IsPointerOverScrollbar_(const UiInputFrame& frame) const noexcept;

		[[nodiscard]] bool IsPointerInsideDropdown_(float x, float y) const noexcept;

		[[nodiscard]] int HitTestItemIndex_(float x, float y) const noexcept;



		void SetExpanded_(bool expanded) noexcept;

		void ToggleExpanded_() noexcept;

		void Collapse_(FocusManager& focus) noexcept;

		void ReleaseFocusIfHeld_(FocusManager& focus) noexcept;

		void TryCollapseOnExternalInteraction_(const UiInputFrame& frame, FocusManager& focus) noexcept;



		void UpdateHighlightFromPointer_(const UiInputFrame& frame) noexcept;

		void MoveListHighlight_(int delta) noexcept;

		void HandleExpandedListNavigation_(const UiInputFrame& frame) noexcept;

		void HandleListScrollInput_(const UiInputFrame& frame) noexcept;

		void UpdateScrollbarInteraction_(const UiInputFrame& frame) noexcept;

		void ApplyScrollFromPointerY_(float pointerY) noexcept;



		FocusHandle focusHandle_;



		UiRect headerBounds_{};

		UiRect listBounds_{};

		UiRect scrollbarTrackBounds_{};

		UiRect scrollbarThumbBounds_{};



		std::vector<UiRect> itemBounds_;

		std::vector<DropdownOption> options_;



		int selectedIndex_ = 0;

		int highlightIndex_ = -1;

		int scrollOffset_ = 0;



		float itemHeight_ = 1.0f;

		float listOffsetY_ = 0.0f;

		float scrollbarWidth_ = 12.0f;



		unsigned maxListVisibleItems_ = 8u;



		bool enabled_ = true;

		bool expanded_ = false;

		bool trackingPointerPress_ = false;

		bool trackingScrollbarDrag_ = false;

		bool pointerWasDownLastFrame_ = false;

		bool keyboardListNavPrimed_ = false;

		bool scrollbarHovered_ = false;



		UiVisualPhase headerPhase_ = UiVisualPhase::Normal;



		std::function<void(int index, const std::string& label)> onValueChanged_;

	};

}


