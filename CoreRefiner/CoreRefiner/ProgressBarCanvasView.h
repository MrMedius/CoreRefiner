#pragma once

#include "IUiView.h"
#include "ProgressBarViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"

#include <memory>

namespace Ui
{
	struct ProgressBarCanvasStyle
	{
		Color trackColor = Color(45u, 45u, 48u, 255u);
		Color fillColor = Color(25u, 110u, 200u, 255u);
		Color borderColor = Color(80u, 80u, 85u, 255u);
		Color indeterminateColor = Color(120u, 200u, 255u, 255u);
		unsigned paddingPx = 4u;
		unsigned borderPx = 1u;
	};

	class ProgressBarCanvasView final : public IUiView
	{
	public:
		ProgressBarCanvasView(Graphics& gfx, unsigned pixelWidth, unsigned pixelHeight, ProgressBarCanvasStyle style = {});

		void SyncFrom(const ProgressBarViewModel& vm);
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

		[[nodiscard]] Canvas2D& GetCanvas() noexcept { return *canvas_; }
		[[nodiscard]] const Canvas2D& GetCanvas() const noexcept { return *canvas_; }

	private:
		void Repaint_(const ProgressBarViewModel& vm);

		ProgressBarCanvasStyle style_;
		std::unique_ptr<Canvas2D> canvas_;

		ProgressBarViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}
