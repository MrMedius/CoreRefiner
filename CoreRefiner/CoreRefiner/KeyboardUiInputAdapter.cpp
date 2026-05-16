#include "KeyboardUiInputAdapter.h"

#include "InputCodex.h"
#include "Win.h"

namespace Ui
{
	UiInputFrame KeyboardUiInputAdapter::BuildFrame() const
	{
		UiInputFrame frame{};
		const InputCodex& in = InputCodex::Get();

		if (in.KeyTriggered(VK_TAB))
		{
			if (in.KeyPressed(VK_SHIFT))
				frame.navigation.tabPrev = true;
			else
				frame.navigation.tabNext = true;
		}
		if (in.KeyTriggered(VK_UP) || in.KeyTriggered(KK_W))
			frame.navigation.tabPrev = true;
		if (in.KeyTriggered(VK_DOWN) || in.KeyTriggered(KK_S))
			frame.navigation.tabNext = true;

		if (in.KeyTriggered(VK_RETURN) || in.KeyTriggered(VK_SPACE))
			frame.action.confirmPressed = true;
		if (in.KeyTriggered(VK_ESCAPE))
			frame.action.cancelPressed = true;

		return frame;
	}
}