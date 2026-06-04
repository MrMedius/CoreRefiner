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

		// 文本输入激活时，W/S 用于打字，不再充当导航；方向键始终可切换控件。
		const bool textCapture = in.TextCaptureRequested();
		if ((!textCapture && in.KeyTriggered(KK_S)) || in.KeyTriggered(VK_DOWN))
			frame.navigation.navDown = true;
		if ((!textCapture && in.KeyTriggered(KK_W)) || in.KeyTriggered(VK_UP))
			frame.navigation.navUp = true;


		const bool confirmDown = in.KeyPressed(VK_RETURN) || in.KeyPressed(VK_SPACE);
		if (confirmDown)
			frame.action.confirmDown = true;
		if (in.KeyTriggered(VK_RETURN) || in.KeyTriggered(VK_SPACE))
			frame.action.confirmPressed = true;
		if (in.KeyReleased(VK_RETURN) || in.KeyReleased(VK_SPACE))
			frame.action.confirmReleased = true;
		if (in.KeyTriggered(VK_ESCAPE))
			frame.action.cancelPressed = true;

		return frame;
	}
}