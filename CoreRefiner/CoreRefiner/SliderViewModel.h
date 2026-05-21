#pragma once
#include "UiSlider.h"
#include "UiVisualPhase.h"

#include <cmath>

namespace Ui
{
	struct SliderViewModel
	{
		float normalized = 0.0f;
		UiVisualPhase phase = UiVisualPhase::Normal;
		bool enabled = true;
		bool interactive = true;
		float outerCenterX = 0.0f;
		float outerCenterY = 0.0f;
		float outerWidth = 1.0f;
		float outerHeight = 1.0f;
		float grooveCenterX = 0.0f;
		float grooveCenterY = 0.0f;
		float grooveWidth = 1.0f;
		float grooveHeight = 1.0f;
		float rotationRadZ = 0.0f;
		float rotationDegZ = 0.0f;
	};

	inline SliderViewModel MakeSliderViewModel(const UiSlider& slider)
	{
		const SliderGrooveLayout& layout = slider.GetGrooveLayout();
		const float rotationRad = layout.rotationRadZ;
		const float rotationDeg = rotationRad * (180.0f / 3.14159265358979323846f);

		return SliderViewModel{
			.normalized = slider.GetNormalized(),
			.phase = slider.GetVisualPhase(),
			.enabled = slider.IsEnabled(),
			.interactive = slider.IsInteractive(),
			.outerCenterX = layout.centerX,
			.outerCenterY = layout.centerY,
			.outerWidth = layout.outerWidth,
			.outerHeight = layout.outerHeight,
			.grooveCenterX = layout.centerX,
			.grooveCenterY = layout.centerY,
			.grooveWidth = layout.grooveWidth,
			.grooveHeight = layout.grooveHeight,
			.rotationRadZ = rotationRad,
			.rotationDegZ = rotationDeg
		};
	}
}