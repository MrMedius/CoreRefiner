#include "UiProgressBar.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	UiProgressBar::UiProgressBar(UiRect bounds)
		:
		bounds_(bounds)
	{}

	void UiProgressBar::SetRange(const float minValue, const float maxValue) noexcept
	{
		min_ = minValue;
		max_ = (maxValue > minValue) ? maxValue : minValue + 1.0f;
		value_ = std::clamp(value_, min_, max_);
	}

	void UiProgressBar::SetValue(const float value) noexcept
	{
		value_ = std::clamp(value, min_, max_);
	}

	float UiProgressBar::GetNormalized() const noexcept
	{
		const float span = max_ - min_;
		if (span <= 0.0f)
			return 0.0f;
		return std::clamp((value_ - min_) / span, 0.0f, 1.0f);
	}

	void UiProgressBar::Update(const UiInputFrame& /*frame*/, const FocusManager& /*focus*/)
	{
	}
}
