#pragma once

#include "UiInputFrame.h"

namespace Ui
{
	class MouseUiInputAdapter
	{
	public:
		MouseUiInputAdapter() = default;

		[[nodiscard]] UiInputFrame BuildFrame(bool respectImGuiCapture = true) const;
	};
}