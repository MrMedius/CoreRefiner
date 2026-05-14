#pragma once

#include "FocusTypes.h"
#include "UiInputFrame.h"
#include "UiTypes.h"

#include <functional>
#include <string>


namespace Ui
{
	class FocusManager;

	/**
	 * @brief 按钮在表现层使用的视觉阶段（与设备无关）。
	 */
	enum class ButtonVisualPhase
	{
		Normal,
		Hovered,
		Focused,
		Pressed,
		Disabled
	};

	/**
	 * @class UiButton
	 * @brief 仅交互逻辑：命中、按压、点击、焦点+确认；不包含任何 Drawable / 纹理。
	 */
	class UiButton
	{
	public:
		/**
		 * @param focusHandle 非 0；由上层分配并注册到 `FocusManager`。
		 * @param bounds 逻辑空间 AABB，与 `UiInputFrame` 指针坐标同空间。
		 */
		UiButton(FocusHandle focusHandle, UiRect bounds);

		void SetBounds(UiRect r) noexcept { bounds_ = r; }
		[[nodiscard]] const UiRect& GetBounds() const noexcept { return bounds_; }

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept { return focusHandle_; }

		void SetEnabled(bool enabled) noexcept;
		[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

		void SetLabel(std::string utf8) { labelUtf8_ = std::move(utf8); }
		[[nodiscard]] const std::string& GetLabel() const noexcept { return labelUtf8_; }

		void SetOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

		/**
		 * @brief 每帧调用：处理指针与焦点，可能触发 `onClick`。
		 */
		void Update(const UiInputFrame& frame, const FocusManager& focus);

		/**
		 * @brief 供 View 拉取当前视觉阶段与标签（第六步 `ButtonViewModel`）。
		 */
		[[nodiscard]] ButtonVisualPhase GetVisualPhase() const noexcept { return visualPhase_; }

		/** @brief 鼠标夺回主导时：清按下跟踪，避免残留 Pressed。 */
		void ResetPointerInteraction() noexcept;

	private:
		[[nodiscard]] bool IsPointerOver(const UiInputFrame& frame) const noexcept;
		void RecomputeVisualPhase(const UiInputFrame& frame, const FocusManager& focus) noexcept;

		FocusHandle focusHandle_;
		UiRect bounds_{};
		bool enabled_ = true;
		std::string labelUtf8_;

		/** 本按钮发起的左键按下跟踪（内按下后直到左键释放）。 */
		bool trackingPointerPress_ = false;

		ButtonVisualPhase visualPhase_ = ButtonVisualPhase::Normal;

		std::function<void()> onClick_;

		bool pointerWasDownLastFrame_ = false;
	};
}