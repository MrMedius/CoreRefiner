#pragma once

#include "UiDropdown.h"
#include "UiVisualPhase.h"

#include <string>

namespace Ui
{
	/** @brief Dropdown 视图快照（Step 3 仅绘制折叠 Header）。 */
	struct DropdownViewModel
	{
		std::string selectedLabel;
		UiVisualPhase headerPhase = UiVisualPhase::Normal;
		bool expanded = false;
		bool enabled = true;
	};

	/** @brief 从 UiDropdown 生成 ViewModel。 */
	inline DropdownViewModel MakeDropdownViewModel(const UiDropdown& dropdown)
	{
		return DropdownViewModel{
			.selectedLabel = dropdown.GetSelectedLabel(),
			.headerPhase = dropdown.GetHeaderVisualPhase(),
			.expanded = dropdown.IsExpanded(),
			.enabled = dropdown.IsEnabled()
		};
	}
}
