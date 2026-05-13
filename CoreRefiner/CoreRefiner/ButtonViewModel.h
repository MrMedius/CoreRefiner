#pragma once

#include "UiButton.h"

#include <string>

namespace Ui
{
	/**
	 * @brief 表现层只读快照；由 `UiButton` 填充或由调用方手工组装。
	 */
	struct ButtonViewModel
	{
		ButtonVisualPhase phase = ButtonVisualPhase::Normal;
		std::string labelUtf8;
	};

	inline ButtonViewModel MakeButtonViewModel(const UiButton& b)
	{
		return ButtonViewModel{ .phase = b.GetVisualPhase(), .labelUtf8 = b.GetLabel() };
	}
}