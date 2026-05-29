#pragma once
#include "FocusTypes.h"
#include "IUiLogic.h"
#include "UiOptionList.h"
#include "UiPointerPressTracker.h"
#include "UiTypes.h"
#include "UiVisualPhase.h"
#include <functional>
#include <string>
#include <vector>

namespace Ui
{
	class FocusManager;

	class UiDropdown : public IUiLogic
	{
	public:
		UiDropdown(FocusHandle focusHandle, UiRect headerBounds);
		void SetHeaderBounds(UiRect r) noexcept;
		[[nodiscard]] const UiRect& GetHeaderBounds() const noexcept { return headerBounds_; }
		void SetItemHeight(float logicalHeight) noexcept;
		[[nodiscard]] float GetItemHeight() const noexcept { return itemHeight_; }
		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }
		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void BindOptionList(UiOptionList& list) noexcept { optionList_ = &list; }
		void UnbindOptionList() noexcept { optionList_ = &ownedList_; }
		[[nodiscard]] UiOptionList& OptionList() noexcept { return *optionList_; }
		[[nodiscard]] const UiOptionList& OptionList() const noexcept { return *optionList_; }

		void SetOptions(std::vector<UiOption> options);
		void AddOptions(std::vector<UiOption> options);
		void AddOption(UiOption option);
		void EraseOptions(std::vector<int> indices);
		void EraseOption(int index);
		void ClearOptions() noexcept;
		[[nodiscard]] const std::vector<UiOption>& GetOptions() const noexcept
		{
			return optionList_->GetOptions();
		}
		[[nodiscard]] int GetSelectedIndex() const noexcept { return optionList_->GetSelectedIndex(); }
		void SetSelectedIndex(int index, bool notify = true) noexcept;
		[[nodiscard]] int GetHighlightIndex() const noexcept { return highlightIndex_; }
		[[nodiscard]] UiVisualPhase GetListItemVisualPhase(int index) const noexcept;
		[[nodiscard]] std::string GetSelectedLabel() const;
		[[nodiscard]] bool IsExpanded() const noexcept { return expanded_; }
		[[nodiscard]] bool ConsumesDirectionalNavigation() const noexcept;
		[[nodiscard]] bool ClaimsPointerInteraction(float x, float y) const noexcept;
		[[nodiscard]] bool BlocksUnderlyingPointerAt(float x, float y) const noexcept;
		[[nodiscard]] float GetListOffsetY() const noexcept { return listOffsetY_; }
		[[nodiscard]] float GetListHeight() const noexcept;
		void SetOnValueChanged(std::function<void(int index, const std::string& label)> cb)
		{
			onValueChanged_ = std::move(cb);
		}

		void Update(const UiInputFrame& frame, FocusManager& focus) override;
		[[nodiscard]] UiVisualPhase GetHeaderVisualPhase() const noexcept { return headerPhase_; }
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }
	private:
		void RebuildItemBounds_() noexcept;
		void RecomputeListEdgeOffset_() noexcept;
		void NormalizeSelectionAfterOptionsChange_() noexcept;
		void RecomputeHeaderPhase_(const UiInputFrame& frame, FocusManager& focus) noexcept;
		[[nodiscard]] bool IsPointerOverHeader_(const UiInputFrame& frame) const noexcept;
		[[nodiscard]] bool IsPointerOverList_(const UiInputFrame& frame) const noexcept;
		[[nodiscard]] bool IsPointerInsideDropdown_(float x, float y) const noexcept;
		[[nodiscard]] int HitTestItemIndex_(float x, float y) const noexcept;
		void SetExpanded_(bool expanded) noexcept;
		void ToggleExpanded_() noexcept;
		void Collapse_() noexcept;
		void CollapseFromPointer_(FocusManager& focus) noexcept;
		void ReleaseFocusIfHeld_(FocusManager& focus) noexcept;
		void TryCollapseOnExternalInteraction_(const UiInputFrame& frame, FocusManager& focus) noexcept;
		void UpdateHighlightFromPointer_(const UiInputFrame& frame) noexcept;
		void MoveListHighlight_(int delta) noexcept;
		void HandleExpandedListNavigation_(const UiInputFrame& frame) noexcept;
		void RecomputeListItemPhases_(const UiInputFrame& frame) noexcept;
		[[nodiscard]] bool IsListItemPressed_(int index, const UiInputFrame& frame) const noexcept;

	private:
		FocusHandle focusHandle_;
		UiRect headerBounds_{};
		UiRect listBounds_{};
		std::vector<UiRect> itemBounds_;
		UiOptionList ownedList_;
		UiOptionList* optionList_ = &ownedList_;
		int highlightIndex_ = -1;
		float itemHeight_ = 1.0f;
		float listOffsetY_ = 0.0f;
		bool enabled_ = true;
		bool expanded_ = false;
		UiPointerPressTracker headerPress_;
		UiIndexedPointerPressTracker listPress_;
		bool keyboardListNavPrimed_ = false;
		std::vector<UiVisualPhase> listItemPhases_;
		UiVisualPhase headerPhase_ = UiVisualPhase::Normal;
		std::function<void(int index, const std::string& label)> onValueChanged_;
	};
}