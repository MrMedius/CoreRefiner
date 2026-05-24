#pragma once

#include "FocusTypes.h"
#include "IUiLogic.h"
#include "UiTypes.h"
#include "UiVisualPhase.h"

#include <functional>
#include <string>
#include <vector>

namespace Ui
{
	class FocusManager;

	/** @brief Dropdown 单个选项（逻辑层数据，与渲染后端无关）。 */
	struct DropdownOption
	{
		std::string label;
	};

	/**
	 * @brief Dropdown 逻辑层（Step 3：固定选项 + Header 视觉相位，暂不展开）。
	 */
	class UiDropdown : public IUiLogic
	{
	public:
		UiDropdown(FocusHandle focusHandle, UiRect headerBounds);

		void SetHeaderBounds(UiRect r) noexcept { headerBounds_ = r; }
		[[nodiscard]] const UiRect& GetHeaderBounds() const noexcept { return headerBounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void SetOptions(std::vector<DropdownOption> options);
		[[nodiscard]] const std::vector<DropdownOption>& GetOptions() const noexcept { return options_; }

		[[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex_; }
		void SetSelectedIndex(int index, bool notify = true) noexcept;

		[[nodiscard]] std::string GetSelectedLabel() const;
		[[nodiscard]] bool IsExpanded() const noexcept { return expanded_; }

		void SetOnValueChanged(std::function<void(int index, const std::string& label)> cb)
		{
			onValueChanged_ = std::move(cb);
		}

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		[[nodiscard]] UiVisualPhase GetHeaderVisualPhase() const noexcept { return headerPhase_; }

		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override { return enabled_; }

	private:
		void RecomputeHeaderPhase_(const UiInputFrame& frame, const FocusManager& focus) noexcept;
		[[nodiscard]] bool IsPointerOverHeader_(const UiInputFrame& frame) const noexcept;

		FocusHandle focusHandle_;
		UiRect headerBounds_{};
		std::vector<DropdownOption> options_;
		int selectedIndex_ = 0;
		bool enabled_ = true;
		bool expanded_ = false;

		bool trackingPointerPress_ = false;
		bool pointerWasDownLastFrame_ = false;
		UiVisualPhase headerPhase_ = UiVisualPhase::Normal;

		std::function<void(int index, const std::string& label)> onValueChanged_;
	};
}
