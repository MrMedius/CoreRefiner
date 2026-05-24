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

	/** @brief Dropdown 逻辑：展开/折叠、鼠标点选、列表 hit-test。 */
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


		void SetOptions(std::vector<DropdownOption> options);

		[[nodiscard]] const std::vector<DropdownOption>& GetOptions() const noexcept { return options_; }



		[[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex_; }

		void SetSelectedIndex(int index, bool notify = true) noexcept;



		[[nodiscard]] int GetHighlightIndex() const noexcept { return highlightIndex_; }

		[[nodiscard]] std::string GetSelectedLabel() const;

		[[nodiscard]] bool IsExpanded() const noexcept { return expanded_; }



		void SetOnValueChanged(std::function<void(int index, const std::string& label)> cb)

		{

			onValueChanged_ = std::move(cb);

		}



		void Update(const UiInputFrame& frame, const FocusManager& focus) override;

		[[nodiscard]] UiVisualPhase GetHeaderVisualPhase() const noexcept { return headerPhase_; }



		void ResetPointerInteraction() noexcept override;

		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }



	private:

		void RebuildItemBounds_() noexcept;

		void RecomputeHeaderPhase_(const UiInputFrame& frame, const FocusManager& focus) noexcept;

		[[nodiscard]] bool IsPointerOverHeader_(const UiInputFrame& frame) const noexcept;

		[[nodiscard]] bool IsPointerOverList_(const UiInputFrame& frame) const noexcept;

		[[nodiscard]] int HitTestItemIndex_(float x, float y) const noexcept;

		void SetExpanded_(bool expanded) noexcept;

		void ToggleExpanded_() noexcept;

		void Collapse_() noexcept;

		void UpdateHighlightFromPointer_(const UiInputFrame& frame) noexcept;



		FocusHandle focusHandle_;

		UiRect headerBounds_{};

		UiRect listBounds_{};

		std::vector<UiRect> itemBounds_{};

		std::vector<DropdownOption> options_;

		int selectedIndex_ = 0;

		int highlightIndex_ = -1;

		float itemHeight_ = 1.0f;

		bool enabled_ = true;

		bool expanded_ = false;



		bool trackingPointerPress_ = false;

		bool pointerWasDownLastFrame_ = false;

		UiVisualPhase headerPhase_ = UiVisualPhase::Normal;



		std::function<void(int index, const std::string& label)> onValueChanged_;

	};

}

