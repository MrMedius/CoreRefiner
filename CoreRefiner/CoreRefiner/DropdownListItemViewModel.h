#pragma once

#include <string>

namespace Ui
{
	/** @brief Dropdown 列表单行视图快照。 */
	struct DropdownListItemViewModel
	{
		std::string label;
		bool highlighted = false;
		bool selected = false;
	};
}
