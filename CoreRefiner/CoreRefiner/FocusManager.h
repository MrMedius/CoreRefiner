#pragma once

#include "FocusTypes.h"
#include "UiInputFrame.h"

#include <cstddef>
#include <vector>

namespace Ui
{
	class FocusManager
	{
	public:
		FocusManager() = default;

		/** @brief 替换整段 Tab 序；若当前焦点不在新表中则清除焦点。 */
		void SetTabOrder(std::vector<FocusHandle> order);

		/** @brief 在 Tab 序末尾追加一项（去重：已存在则移到末尾或不重复添加，此处实现为若已存在则不重复 push）。 */
		void RegisterTabStop(FocusHandle h);

		/** @brief 从 Tab 序中移除；若移除的是当前焦点则 `ClearFocus`。 */
		void UnregisterTabStop(FocusHandle h);

		void ClearTabOrder() noexcept;

		/** @brief 清除当前焦点，不改动 Tab 表。 */
		void ClearFocus() noexcept;

		/** @brief 若 `h` 非 0 则设为当前焦点（不校验是否在 Tab 序中）。 */
		void RequestFocus(FocusHandle h) noexcept;

		FocusHandle Focused() const noexcept { return focused_; }

		bool IsFocused(FocusHandle h) const noexcept
		{
			return h != kInvalidFocusHandle && focused_ == h;
		}

		/** @brief 根据本帧语义导航边沿切换焦点（通常消费 `UiNavigationPayload`）。 */
		void ApplyNavigation(const UiInputFrame& frame);

		/** @brief 在 Tab 序中前进一环；序为空则无操作。 */
		void FocusNext() noexcept;

		/** @brief 在 Tab 序中后退一环。 */
		void FocusPrev() noexcept;

	private:
		std::ptrdiff_t FindIndex(FocusHandle h) const noexcept;
		void FocusAtIndex(std::size_t index) noexcept;

		std::vector<FocusHandle> tabOrder_{};
		FocusHandle focused_ = kInvalidFocusHandle;
	};
}