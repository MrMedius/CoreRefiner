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

	class ButtonCanvasComponent
	{
	public:
		ButtonCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			unsigned canvasPixelWidth,
			unsigned canvasPixelHeight,
			ButtonCanvasStyle style = {});

		ButtonCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			float centerX, 
			float centerY, 
			float width, 
			float height,
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

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float height) noexcept;

		void SetLayoutLogicalRect(const UiRect& r) noexcept;

		void SyncViewFromButton() const;

		void RegisterTo(UiRoot& root) const;

		void LinkTechniques(Rgph::RenderGraph& rg) const;

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiButton> button_;
		std::unique_ptr<ButtonCanvasView> view_;
	};
}