#pragma once
#include "UiTypes.h"

#include <string>
#include <vector>

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
		bool confirmDown = false;

		bool confirmPressed = false;

		bool confirmReleased = false;
		bool cancelPressed = false;
	};

	struct UiTextInputPayload
	{
		std::vector<std::string> commitUtf8;
		std::string imeCompositionUtf8;
		bool imeCompositionActive = false;
	};

	struct UiInputFrame
	{
		UiPointerPayload pointer{};
		UiNavigationPayload navigation{};
		UiActionPayload action{};
		UiTextInputPayload text{};
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

		acc.action.confirmDown = acc.action.confirmDown || layer.action.confirmDown;
		acc.action.confirmPressed = acc.action.confirmPressed || layer.action.confirmPressed;
		acc.action.confirmReleased = acc.action.confirmReleased || layer.action.confirmReleased;
		acc.action.cancelPressed = acc.action.cancelPressed || layer.action.cancelPressed;

		acc.text.commitUtf8.insert(
			acc.text.commitUtf8.end(),
			layer.text.commitUtf8.begin(),
			layer.text.commitUtf8.end());
		if (layer.text.imeCompositionActive)
		{
			acc.text.imeCompositionUtf8 = layer.text.imeCompositionUtf8;
			acc.text.imeCompositionActive = true;
		}
	}
}