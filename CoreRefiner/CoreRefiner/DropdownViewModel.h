#pragma once
#include "UiDropdown.h"
#include "UiVisualPhase.h"
#include <string>
#include <vector>
namespace Ui
{
	struct DropdownViewModel
	{
		std::string selectedLabel;
		UiVisualPhase headerPhase = UiVisualPhase::Normal;
		bool expanded = false;
		bool enabled = true;
		int selectedIndex = 0;
		int highlightIndex = -1;
		std::vector<std::string> optionLabels;
		std::vector<UiVisualPhase> listItemPhases;
		float itemHeight = 1.0f;

		float listOffsetY = 0.0f;
		float listHeight = 0.0f;
	};

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
			.listHeight = dropdown.GetListHeight()
		};

		const auto& options = dropdown.GetOptions();
		vm.optionLabels.reserve(options.size());
		vm.listItemPhases.reserve(options.size());
		for (int i = 0; i < static_cast<int>(options.size()); ++i)
		{
			vm.optionLabels.push_back(options[static_cast<size_t>(i)].label);
			vm.listItemPhases.push_back(dropdown.GetListItemVisualPhase(i));
		}
		return vm;
	}

}
