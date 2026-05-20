#pragma once

#include "UiProgressBar.h"

namespace Ui
{
	struct ProgressBarViewModel
	{
		float normalized = 0.0f;
		bool indeterminate = false;
		bool enabled = true;
	};

	inline ProgressBarViewModel MakeProgressBarViewModel(const UiProgressBar& bar)
	{
		return ProgressBarViewModel{
			.normalized = bar.GetNormalized(),
			.indeterminate = bar.IsIndeterminate(),
			.enabled = bar.IsEnabled()
		};
	}
}
