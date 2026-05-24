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

			.listOffsetY = dropdown.GetListOffsetY()

		};



		const auto& options = dropdown.GetOptions();

		vm.optionLabels.reserve(options.size());

		for (const DropdownOption& opt : options)

			vm.optionLabels.push_back(opt.label);



		return vm;

	}

}

