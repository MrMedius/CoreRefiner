#pragma once
#include "UiInputFrame.h"
namespace Ui
{
	class FocusManager;

	class IUiPopupConsumer
	{
	public:
		virtual ~IUiPopupConsumer() = default;
		[[nodiscard]] virtual bool IsPopupOpen() const noexcept = 0;

		[[nodiscard]] virtual bool BlocksUnderlyingPointerAt(float x, float y) const noexcept
		{
			return false;
		}

		virtual void OnPopupInput(UiInputFrame& frame, FocusManager& focus) = 0;
	};

}

