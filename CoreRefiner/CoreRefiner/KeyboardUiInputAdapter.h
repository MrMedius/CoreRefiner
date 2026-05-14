#pragma once

#include "UiInputFrame.h"

namespace Ui
{
	/**
	 * @class KeyboardUiInputAdapter
	 * @brief 从 `InputCodex` 读键盘，只写 `navigation` / `action`（不写 `pointer`）。
	 *
	 * @par 映射
	 * - Tab / Shift+Tab：tabNext / tabPrev
	 * - 左/右方向键：tabPrev / tabNext
	 * - Enter / 空格：confirmPressed
	 * - Esc：cancelPressed
	 */
	class KeyboardUiInputAdapter
	{
	public:
		KeyboardUiInputAdapter() = default;

		[[nodiscard]] UiInputFrame BuildFrame() const;
	};
}