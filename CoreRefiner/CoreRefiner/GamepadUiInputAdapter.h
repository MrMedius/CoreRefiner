#pragma once

#include "UiInputFrame.h"

namespace Ui
{
	/**
	 * @class GamepadUiInputAdapter
	 * @brief 从 `InputCodex` 读指定 XInput 槽位，只写 `navigation` / `action`。
	 *
	 * @par 默认映射
	 * - LB / RB 边沿：tabPrev / tabNext
	 * - 十字左/右：tabPrev / tabNext
	 * - A：confirmPressed；B：cancelPressed
	 */
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