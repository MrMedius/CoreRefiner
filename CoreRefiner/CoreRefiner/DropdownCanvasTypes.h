#pragma once

#include "Colors.h"
#include "TextTypes.h"
#include "UiOptionList.h"

#include <functional>

class Canvas;

namespace Ui
{
	struct DropdownCanvasStyle
	{
		Color headerNormal = Color(45u, 45u, 48u, 255u);
		Color headerFocused = Color(35u, 55u, 95u, 255u);
		Color headerPressed = Color(25u, 110u, 200u, 255u);
		Color headerDisabled = Color(55u, 55u, 55u, 255u);

		Color headerBorder = Color(80u, 80u, 85u, 255u);
		Color headerTextNormal = Colors::White;
		Color headerTextDisabled = Color(130u, 130u, 130u, 255u);
		Color arrowColor = Color(200u, 200u, 205u, 255u);

		Color itemNormal = Color(40u, 40u, 44u, 255u);
		Color itemHighlight = Color(55u, 75u, 110u, 255u);
		Color itemSelected = Color(35u, 65u, 105u, 255u);
		Color itemPressed = Color(25u, 110u, 200u, 255u);
		Color itemFocusRing = Color(120u, 200u, 255u, 255u);
		Color itemText = Colors::White;

		unsigned headerBorderPx = 1u;
		unsigned headerPaddingPx = 8u;
		unsigned arrowWidthPx = 16u;
		unsigned itemFocusRingThicknessPx = 3u;

		Text::FontSource primaryFont = Text::FontSource::System(L"Segoe UI");
		float fontSize = 18.0f;
	};

	using DropdownCanvasItemDrawFn = std::function<void(
		::Canvas& canvas,
		const UiOption& option,
		unsigned itemIndex,
		bool highlighted,
		bool selected)>;
}
