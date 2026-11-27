#pragma once
#include "UiStepper.h"
#include "UiVisualPhase.h"

#include <string>

namespace Ui
{
	struct StepperViewModel
	{
		UiVisualPhase minusPhase    = UiVisualPhase::Normal;
		UiVisualPhase plusPhase     = UiVisualPhase::Normal;
		bool          showFocusRing = false;
		bool          enabled       = true;
		bool          showCenterText = true;
		float         centerGapWidth = 0.0f;
		std::string   displayText;
	};

	inline StepperViewModel MakeStepperViewModel(const UiStepper& s)
	{
		return StepperViewModel{
			.minusPhase    = s.GetMinusPhase(),
			.plusPhase     = s.GetPlusPhase(),
			.showFocusRing = s.GetShowFocusRing(),
			.enabled       = s.IsEnabled(),
			.displayText   = s.GetLabel()
		};
	}
}
