#include "GamepadUiInputAdapter.h"

#include "InputCodex.h"

namespace Ui
{
	GamepadUiInputAdapter::GamepadUiInputAdapter(int playerIndex) noexcept
		:
		padIndex_{ playerIndex }
	{}

	void GamepadUiInputAdapter::SetPlayerIndex(int idx) noexcept
	{
		padIndex_ = idx;
	}

	UiInputFrame GamepadUiInputAdapter::BuildFrame() const
	{
		UiInputFrame frame{};
		const InputCodex& in = InputCodex::Get();
		if (!in.PadConnected(padIndex_))
			return frame;

		if (in.GP_Triggered(padIndex_, Gamepad::GP_LB))
			frame.navigation.tabPrev = true;
		if (in.GP_Triggered(padIndex_, Gamepad::GP_RB))
			frame.navigation.tabNext = true;

		if (in.GP_Triggered(padIndex_, Gamepad::GP_DPAD_LEFT))
			frame.navigation.tabPrev = true;
		if (in.GP_Triggered(padIndex_, Gamepad::GP_DPAD_RIGHT))
			frame.navigation.tabNext = true;

		if (in.GP_Triggered(padIndex_, Gamepad::GP_A))
			frame.action.confirmPressed = true;
		if (in.GP_Triggered(padIndex_, Gamepad::GP_B))
			frame.action.cancelPressed = true;

		return frame;
	}
}