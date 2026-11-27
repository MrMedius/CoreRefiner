#pragma once

#include "UiToggle.h"
#include "UiVisualPhase.h"

namespace Ui
{
	struct ToggleViewModel
	{
		bool isOn = false;
		UiVisualPhase phase = UiVisualPhase::Normal;
		bool enabled = true;
	};

	inline ToggleViewModel MakeToggleViewModel(const UiToggle& toggle)
	{
		return ToggleViewModel{
			.isOn = toggle.IsOn(),
			.phase = toggle.GetVisualPhase(),
			.enabled = toggle.IsEnabled()
		};
	}
}
