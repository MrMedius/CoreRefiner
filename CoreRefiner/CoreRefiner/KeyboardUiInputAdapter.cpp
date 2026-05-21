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

		if (in.KeyTriggered(KK_S) || in.KeyTriggered(VK_DOWN))
			frame.navigation.tabNext = true;
		if (in.KeyTriggered(KK_W) || in.KeyTriggered(VK_UP))
			frame.navigation.tabPrev = true;


		if (in.KeyTriggered(VK_RETURN) || in.KeyTriggered(VK_SPACE))
			frame.action.confirmPressed = true;
		if (in.KeyTriggered(VK_ESCAPE))
			frame.action.cancelPressed = true;

		return frame;
	}
}