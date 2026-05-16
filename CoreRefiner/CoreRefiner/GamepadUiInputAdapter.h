#pragma once

#include "UiInputFrame.h"

namespace Ui
{
	class GamepadUiInputAdapter
	{
	public:
		explicit GamepadUiInputAdapter(int playerIndex = 0) noexcept;

		void SetPlayerIndex(int idx) noexcept;

		[[nodiscard]] UiInputFrame BuildFrame() const;

	private:
		int padIndex_{ 0 };
	};
}