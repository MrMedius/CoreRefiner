#pragma once

#include "UiInputFrame.h"

namespace Ui
{
	class KeyboardUiInputAdapter
	{
	public:
		KeyboardUiInputAdapter() = default;

		[[nodiscard]] UiInputFrame BuildFrame() const;
	};
}