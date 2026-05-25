#pragma once

#include "UiTypes.h"

namespace Ui
{
	struct UiPointerPayload
	{
		float logicalX = 0.0f;
		float logicalY = 0.0f;

		bool insideLogicalSurface = false;
		bool primaryDown = false;
		bool primaryPressed = false;
		bool primaryReleased = false;
	};

	struct UiNavigationPayload
	{
		bool tabNext = false;
		bool tabPrev = false;
		bool navUp = false;
		bool navDown = false;
	};

	struct UiActionPayload
	{
		bool confirmPressed = false;
		bool cancelPressed = false;
	};

	/** @brief 滚轮输入（Up 为正步进，Down 为负）。 */
	struct UiScrollPayload
	{
		int wheelSteps = 0;
	};

	struct UiInputFrame
	{
		UiPointerPayload pointer{};
		UiNavigationPayload navigation{};
		UiActionPayload action{};
		UiScrollPayload scroll{};
	};

	inline void MergeUiInputFramesOr(UiInputFrame& acc, const UiInputFrame& layer) noexcept
	{
		if (layer.pointer.insideLogicalSurface)
		{
			acc.pointer.logicalX = layer.pointer.logicalX;
			acc.pointer.logicalY = layer.pointer.logicalY;
			acc.pointer.insideLogicalSurface = true;
		}
		acc.pointer.primaryDown = acc.pointer.primaryDown || layer.pointer.primaryDown;
		acc.pointer.primaryPressed = acc.pointer.primaryPressed || layer.pointer.primaryPressed;
		acc.pointer.primaryReleased = acc.pointer.primaryReleased || layer.pointer.primaryReleased;

		acc.navigation.tabNext = acc.navigation.tabNext || layer.navigation.tabNext;
		acc.navigation.tabPrev = acc.navigation.tabPrev || layer.navigation.tabPrev;
		acc.navigation.navUp = acc.navigation.navUp || layer.navigation.navUp;
		acc.navigation.navDown = acc.navigation.navDown || layer.navigation.navDown;

		acc.action.confirmPressed = acc.action.confirmPressed || layer.action.confirmPressed;
		acc.action.cancelPressed = acc.action.cancelPressed || layer.action.cancelPressed;

		acc.scroll.wheelSteps += layer.scroll.wheelSteps;
	}
}