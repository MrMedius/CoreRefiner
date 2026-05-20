#pragma once

#include "SliderAxis.h"
#include "UiSlider.h"
#include "UiVisualPhase.h"

namespace Ui
{
	struct SliderViewModel
	{
		float normalized = 0.0f;
		UiVisualPhase phase = UiVisualPhase::Normal;
		bool enabled = true;
		bool interactive = true;
		SliderAxis axis = SliderAxis::Horizontal;
		float layoutCenterX = 0.0f;
		float layoutCenterY = 0.0f;
		float layoutWidth = 1.0f;
		float layoutHeight = 1.0f;
	};

	inline SliderViewModel MakeSliderViewModel(const UiSlider& slider)
	{
		const UiRect& b = slider.GetBounds();
		return SliderViewModel{
			.normalized = slider.GetNormalized(),
			.phase = slider.GetVisualPhase(),
			.enabled = slider.IsEnabled(),
			.interactive = slider.IsInteractive(),
			.axis = slider.GetAxis(),
			.layoutCenterX = 0.5f * (b.minX + b.maxX),
			.layoutCenterY = 0.5f * (b.minY + b.maxY),
			.layoutWidth = b.maxX - b.minX,
			.layoutHeight = b.maxY - b.minY
		};
	}
}
