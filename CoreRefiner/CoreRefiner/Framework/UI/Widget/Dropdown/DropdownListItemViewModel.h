#pragma once

#include "UiVisualPhase.h"

#include <string>

namespace Ui
{
	struct DropdownListItemViewModel
	{
		std::string label;
		UiVisualPhase phase = UiVisualPhase::Normal;
		bool highlighted = false;
		bool selected = false;
	};
}
