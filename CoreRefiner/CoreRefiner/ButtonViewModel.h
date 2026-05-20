#pragma once

#include "UiButton.h"
#include "UiVisualPhase.h"

#include <string>

namespace Ui
{
	struct ButtonViewModel
	{
		ButtonVisualPhase phase = UiVisualPhase::Normal;
		std::string labelUtf8;
	};

	inline ButtonViewModel MakeButtonViewModel(const UiButton& b)
	{
		return ButtonViewModel{ .phase = b.GetVisualPhase(), .labelUtf8 = b.GetLabel() };
	}
}