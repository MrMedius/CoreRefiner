#pragma once
#include "UiInputFrame.h"
namespace Ui
{
	class FocusManager;
	/** @brief 展开 Popup 时由 UiRoot 优先路由输入的控件接口。 */
	class IUiPopupConsumer
	{
	public:
		virtual ~IUiPopupConsumer() = default;
		[[nodiscard]] virtual bool IsPopupOpen() const noexcept = 0;
		/** @brief Popup 打开时每帧调用；可修改 frame（如清除 Tab 导航）。 */
		virtual void OnPopupInput(UiInputFrame& frame, FocusManager& focus) = 0;
	};

}

