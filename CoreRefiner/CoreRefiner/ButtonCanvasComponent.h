#pragma once

#include "ButtonCanvasView.h"
#include "FocusTypes.h"
#include "UiButton.h"
#include "UiRoot.h"
#include "UiTypes.h"

#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class UiRoot;

	/**
	 * @file ButtonCanvasWidget.h
	 * @brief 将 `FocusHandle`、`UiButton` 与 `ButtonCanvasView` 组合为单一控件入口，并保证命中框与 Canvas 变换一致。
	 *
	 * @par 布局约定（与现有 TITLE 测试一致）
	 * - 逻辑命中使用 `UiRect`（min/max，与 `UiInputFrame` 指针坐标同空间）。
	 * - `Canvas2D` 的 `SetPosition` 为 quad **中心**；`SetScale` 的 `x/y` 为 quad 在 XY 上的**宽高**（`z` 固定为 1）。
	 * - 不支持旋转时由本类从中心+尺寸反推 `UiRect`；若日后为 Canvas 加旋转，应改为从世界矩阵求 AABB 或单独维护命中形状。
	 */
	class ButtonCanvasComponent
	{
	public:
		/**
		 * @brief 构造控件；初始命中框为占位矩形，须立刻调用 `SetLayoutLogicalCenterSize`（或带布局的工厂）再注册到 `UiRoot`。
		 * @param gfx 图形设备。
		 * @param focusHandle 非 0；须与 `FocusManager` Tab 顺序内其它句柄唯一。
		 * @param canvasPixelWidth 离屏画布像素宽（纹理分辨率）。
		 * @param canvasPixelHeight 离屏画布像素高。
		 * @param style 可选绘制风格。
		 */
		ButtonCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			unsigned canvasPixelWidth,
			unsigned canvasPixelHeight,
			ButtonCanvasStyle style = {});

		ButtonCanvasComponent(const ButtonCanvasComponent&) = delete;
		ButtonCanvasComponent& operator=(const ButtonCanvasComponent&) = delete;
		ButtonCanvasComponent(ButtonCanvasComponent&&) noexcept = default;
		ButtonCanvasComponent& operator=(ButtonCanvasComponent&&) noexcept = default;
		~ButtonCanvasComponent() = default;

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept { return focusHandle_; }

		[[nodiscard]] UiButton& Button() noexcept { return *button_; }
		[[nodiscard]] const UiButton& Button() const noexcept { return *button_; }

		[[nodiscard]] ButtonCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const ButtonCanvasView& View() const noexcept { return *view_; }

		/**
		 * @brief 用逻辑空间中心与宽高同步 `UiButton::SetBounds` 与 Canvas 的 `SetPosition`/`SetScale`。
		 * @param centerX 中心 X（与 `InputCodex::MouseX` 同空间）。
		 * @param centerY 中心 Y。
		 * @param width 命中宽度，须 > 0。
		 * @param height 命中高度，须 > 0。
		 */
		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float height) noexcept;

		/** @brief 等价于由 `r` 推导中心与尺寸再调用 `SetLayoutLogicalCenterSize`。 */
		void SetLayoutLogicalRect(const UiRect& r) noexcept;

		/** @brief 将逻辑状态刷到 Canvas（首帧或改标签后若未经过 `UiRoot::TickAfterInput` 可调用）。 */
		void SyncViewFromButton() const;

		/** @brief 向 `UiRoot` 注册槽位（非拥有指针；`UiRoot` 须在控件析构前 `Clear` 或先析构）。 */
		void RegisterTo(UiRoot& root) const;

		/** @brief 转发到内部 `ButtonCanvasView::LinkTechniques`。 */
		void LinkTechniques(Rgph::RenderGraph& rg) const;

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiButton> button_;
		std::unique_ptr<ButtonCanvasView> view_;
	};
}