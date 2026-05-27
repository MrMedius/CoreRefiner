#pragma once

#include "DropdownCanvasTypes.h"
#include "DropdownListItemCanvasView.h"
#include "DropdownViewModel.h"
#include "IUiView.h"

#include "Canvas2D.h"

#include <memory>
#include <vector>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
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
		void RepaintListPanelBackground_();
		void BakeArrowGeometry_();
		void SyncArrowOrientation_(bool expanded) noexcept;
		void ApplyArrowLayout_() noexcept;
		void EnsureListItemCount_(std::size_t count);
		void ApplyListLayout_(const DropdownViewModel& vm);
		void SyncListItems_(const DropdownViewModel& vm);

		DropdownCanvasStyle style_;
		Graphics& gfx_;

		unsigned headerPixelWidth_ = 1u;
		unsigned headerPixelHeight_ = 1u;
		Rgph::RenderGraph* linkedRg_ = nullptr;
		std::unique_ptr<Canvas2D> headerBgCanvas_;
		std::unique_ptr<Canvas2D> headerTextCanvas_;
		std::unique_ptr<Canvas2D> listPanelBgCanvas_;
		std::unique_ptr<Canvas2D> arrowCanvas_;
		std::vector<std::unique_ptr<DropdownListItemCanvasView>> listItems_;

		std::size_t activeListItemCount_ = 0u;

		float layoutCenterX_ = 0.0f;
		float layoutCenterY_ = 0.0f;
		float layoutWidth_ = 1.0f;
		float layoutHeight_ = 1.0f;

		DropdownViewModel lastPainted_{};
		bool hasPainted_ = false;
		bool arrowExpanded_ = false;
		bool listVisible_ = false;
		bool listPanelPainted_ = false;
	};
}
