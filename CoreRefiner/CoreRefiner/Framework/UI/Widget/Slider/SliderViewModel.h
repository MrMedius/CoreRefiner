#pragma once
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
	};

	inline SliderViewModel MakeSliderViewModel(const UiSlider& slider)
	{
		return SliderViewModel{
			.normalized = slider.GetNormalized(),
			.phase = slider.GetVisualPhase(),
			.enabled = slider.IsEnabled(),
			.interactive = slider.IsInteractive()
		};
	}
}