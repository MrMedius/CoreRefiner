#pragma once

#include "DropdownCanvasTypes.h"
#include "DropdownViewModel.h"
#include "IUiView.h"

#include "Canvas2D.h"

#include <memory>

namespace Ui
{
	/** @brief Dropdown Canvas 视图（Header：背景 / 文字 / 箭头 分层）。 */
	class DropdownCanvasView final : public IUiView
	{
	public:
		DropdownCanvasView(
			Graphics& gfx,
			unsigned headerPixelWidth,
			unsigned headerPixelHeight,
			DropdownCanvasStyle style = {});

		void SyncFrom(const DropdownViewModel& vm);
		void ApplyLayout(float centerX, float centerY, float width, float headerHeight) noexcept;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

	private:
		[[nodiscard]] Color HeaderBackgroundForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] Color HeaderTextColorForPhase(UiVisualPhase phase) const noexcept;
		void RepaintHeaderBackground_(UiVisualPhase phase);
		void RepaintHeaderText_(const DropdownViewModel& vm);
		void BakeArrowGeometry_();
		void SyncArrowOrientation_(bool expanded) noexcept;
		void ApplyArrowLayout_() noexcept;

		DropdownCanvasStyle style_;
		std::unique_ptr<Canvas2D> headerBgCanvas_;
		std::unique_ptr<Canvas2D> headerTextCanvas_;
		std::unique_ptr<Canvas2D> arrowCanvas_;

		float layoutCenterX_ = 0.0f;
		float layoutCenterY_ = 0.0f;
		float layoutWidth_ = 1.0f;
		float layoutHeight_ = 1.0f;

		DropdownViewModel lastPainted_{};
		bool hasPainted_ = false;
		bool arrowExpanded_ = false;
	};
}
