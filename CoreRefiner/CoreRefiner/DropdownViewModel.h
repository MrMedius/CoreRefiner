#pragma once



#include "UiDropdown.h"

#include "UiVisualPhase.h"



#include <string>

#include <vector>



namespace Ui

{

	/** @brief Dropdown 视图快照。 */

	struct DropdownViewModel

	{

		std::string selectedLabel;

		UiVisualPhase headerPhase = UiVisualPhase::Normal;

		bool expanded = false;

		bool enabled = true;

		int selectedIndex = 0;

		int highlightIndex = -1;

		std::vector<std::string> optionLabels;

		float itemHeight = 1.0f;

		/** @brief 展开时仅 List 相对默认位置的应用 Y 偏移（Header 不动）。 */
		float listOffsetY = 0.0f;

		/** @brief 列表滚动：首条可见选项在 optionLabels 中的索引。 */
		int scrollOffset = 0;

		/** @brief 当前视口内可见行数（≤ maxListVisibleItems）。 */
		int visibleItemCount = 0;

		bool showScrollbar = false;

		float listViewportHeight = 0.0f;

		float scrollbarWidth = 12.0f;

		float scrollThumbNormalizedPos = 0.0f;

		float scrollThumbNormalizedSize = 1.0f;

		bool scrollbarHovered = false;
	};



	/** @brief 从 UiDropdown 生成 ViewModel。 */

	inline DropdownViewModel MakeDropdownViewModel(const UiDropdown& dropdown)

	{

		DropdownViewModel vm{

			.selectedLabel = dropdown.GetSelectedLabel(),

			.headerPhase = dropdown.GetHeaderVisualPhase(),

			.expanded = dropdown.IsExpanded(),

			.enabled = dropdown.IsEnabled(),

			.selectedIndex = dropdown.GetSelectedIndex(),

			.highlightIndex = dropdown.GetHighlightIndex(),

			.itemHeight = dropdown.GetItemHeight(),

			.listOffsetY = dropdown.GetListOffsetY(),

			.scrollOffset = dropdown.GetScrollOffset(),

			.visibleItemCount = dropdown.GetVisibleItemCount(),

			.showScrollbar = dropdown.GetShowScrollbar(),

			.listViewportHeight = dropdown.GetListViewportHeight(),

			.scrollbarWidth = dropdown.GetScrollbarWidth(),

			.scrollThumbNormalizedPos = dropdown.GetScrollThumbNormalizedPos(),

			.scrollThumbNormalizedSize = dropdown.GetScrollThumbNormalizedSize(),

			.scrollbarHovered = dropdown.IsScrollbarHovered()

		};



		const auto& options = dropdown.GetOptions();

		vm.optionLabels.reserve(options.size());

		for (const DropdownOption& opt : options)

			vm.optionLabels.push_back(opt.label);



		return vm;

	}

}

