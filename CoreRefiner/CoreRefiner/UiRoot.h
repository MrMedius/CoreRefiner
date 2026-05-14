#pragma once

#include "FocusManager.h"
#include "MouseUiInputAdapter.h"
#include "KeyboardUiInputAdapter.h"
#include "GamepadUiInputAdapter.h"

#include <cstddef>
#include <vector>

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class UiButton;
	class IButtonView;


	/**
	 * @brief 当前 UI 由谁主导：鼠标 vs 键盘/手柄（非指针）。
	 */
	enum class UiInputDominance
	{
		Mouse,
		NonPointer
	};

	/**
	 * @struct UiButtonSlot
	 * @brief 非拥有：由外层持有 `UiButton` 与 `IButtonView` 生命周期。
	 */
	struct UiButtonSlot
	{
		UiButton* button = nullptr;
		IButtonView* view = nullptr;
	};

	/**
	 * @class UiRoot
	 * @brief 单帧 UI 流水线：合并语义输入、焦点、Update、Sync、Submit。
	 *
	 * @par 调用约定
	 * - `TickAfterInput()`：同帧须在 `InputCodex::Update()` **之后**调用。
	 * - `Submit()`：在 UI `RenderGraph::Execute` **之前**调用（例如 `Chan::ui`）。
	 *
	 * @par 控件帧与主导权
	 * - 在 `UiInputDominance::NonPointer` 下，下发给控件（如 `UiButton::Update`）的 `UiInputFrame` 会经
	 *   `StripPointerForWidgets_` 清除指针语义（位置、在内表面、按下/边沿等），因此不会出现纯鼠标悬停
	 *   或指针点击；键盘/手柄的 `navigation` / `action` 仍来自合并后的完整帧。
	 * - 输入主导权切换仍基于**未净化**的原始鼠标帧（`mouse_.BuildFrame` 的采样），与控件收到的帧分离，
	 *   避免在 NonPointer 下因“净化后的无指针”而无法切回鼠标主导。
	 */
	class UiRoot
	{
	public:
		UiRoot() = default;

		void Clear() noexcept;

		void AddButtonSlot(UiButton* btn, IButtonView* view);
		void RebuildTabOrderFromSlots();

		void InitLinkTechniques(Rgph::RenderGraph& rg);

		void TickAfterInput();
		void Submit(std::size_t channelMask) const;

		[[nodiscard]] FocusManager& Focus() noexcept { return focus_; }
		[[nodiscard]] const FocusManager& Focus() const noexcept { return focus_; }

		[[nodiscard]] UiInputDominance GetDominance() const noexcept { return dominance_; }

		/** @brief 指定用于 UI 的 XInput 玩家槽（0–3）。 */
		void SetGamepadPlayerIndex(int idx) noexcept { gamepad_.SetPlayerIndex(idx); }

	private:
		/** @brief NonPointer 主导时清空下发给控件的指针负载，保留导航与动作。 */
		static void StripPointerForWidgets_(UiInputFrame& out) noexcept;

		void ResetToMouseDominantState_();
		void ResetToNonPointerDominantState_();

		// Input dominance tracking
		FocusManager focus_{};
		MouseUiInputAdapter mouse_{};
		KeyboardUiInputAdapter keyboard_{};
		GamepadUiInputAdapter gamepad_{ 0 };

		std::vector<UiButtonSlot> slots_{};


		UiInputDominance dominance_{ UiInputDominance::Mouse };

		bool pointerPrimaryWasDown_{ false };

		bool hasLastPointer_{ false };
		float lastPointerLogicalX_{ 0.0f };
		float lastPointerLogicalY_{ 0.0f };
		static constexpr float kMouseMoveDominanceThresholdPx = 2.0f;
	};
}