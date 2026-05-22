#pragma once

#include "UiToggle.h"
#include "UiVisualPhase.h"

#include <algorithm>

namespace Ui
{
	struct ToggleViewModel
	{
		bool isOn = false;
		UiVisualPhase phase = UiVisualPhase::Normal;
		bool enabled = true;
		float layoutCenterX = 0.0f;
		float layoutCenterY = 0.0f;
		float layoutSize = 1.0f;
	};

	inline ToggleViewModel MakeToggleViewModel(const UiToggle& toggle)
	{
		const UiRect& b = toggle.GetBounds();
		const float w = b.maxX - b.minX;
		const float h = b.maxY - b.minY;
		return ToggleViewModel{
			.isOn = toggle.IsOn(),
			.phase = toggle.GetVisualPhase(),
			.enabled = toggle.IsEnabled(),
			.layoutCenterX = 0.5f * (b.minX + b.maxX),
			.layoutCenterY = 0.5f * (b.minY + b.maxY),
			.layoutSize = std::max(w, h)
		};
	}
}
