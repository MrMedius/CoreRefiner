#include "ProgressBarCanvasView.h"

#include "TimeCodex.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	namespace
	{
		void FillRect(Canvas& c, unsigned x0, unsigned y0, unsigned x1, unsigned y1, Color col)
		{
			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			if (w == 0u || h == 0u)
				return;

			const unsigned left = std::min(x0, x1);
			const unsigned right = std::min(std::max(x0, x1), w - 1u);
			const unsigned top = std::min(y0, y1);
			const unsigned bottom = std::min(std::max(y0, y1), h - 1u);

			for (unsigned y = top; y <= bottom; ++y)
				for (unsigned x = left; x <= right; ++x)
					c.PutPixel(x, y, col);
		}
	}

	ProgressBarCanvasView::ProgressBarCanvasView(
		Graphics& gfx,
		const unsigned pixelWidth,
		const unsigned pixelHeight,
		ProgressBarCanvasStyle style)
		:
		style_(std::move(style)),
		canvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight))
	{}

	void ProgressBarCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		canvas_->LinkTechniques(rg);
	}

	void ProgressBarCanvasView::Submit(const std::size_t channelMask) const
	{
		canvas_->Submit(channelMask);
	}

	void ProgressBarCanvasView::Repaint_(const ProgressBarViewModel& vm)
	{
		Canvas& c = *canvas_;
		const unsigned w = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (w == 0u || h == 0u)
			return;

		const Color track = vm.enabled ? style_.trackColor : style_.trackColor * Color(Colors::Gray, 255u);
		const Color fill = vm.enabled ? style_.fillColor : style_.fillColor * Color(Colors::Gray, 255u);

		c.Clear(track);

		const unsigned pad = style_.paddingPx;
		const unsigned border = style_.borderPx;
		if (border > 0u)
		{
			const unsigned x0 = border;
			const unsigned y0 = border;
			const unsigned x1 = (w > border) ? w - 1u - border : 0u;
			const unsigned y1 = (h > border) ? h - 1u - border : 0u;
			FillRect(c, 0u, 0u, w - 1u, border - 1u, style_.borderColor);
			FillRect(c, 0u, h - border, w - 1u, h - 1u, style_.borderColor);
			FillRect(c, 0u, y0, border - 1u, y1, style_.borderColor);
			FillRect(c, w - border, y0, w - 1u, y1, style_.borderColor);
		}

		const unsigned innerLeft = pad + border;
		const unsigned innerTop = pad + border;
		const unsigned innerRight = (w > pad + border) ? w - 1u - pad - border : innerLeft;
		const unsigned innerBottom = (h > pad + border) ? h - 1u - pad - border : innerTop;
		const unsigned innerW = (innerRight >= innerLeft) ? innerRight - innerLeft + 1u : 0u;

		if (innerW == 0u)
			return;

		if (vm.indeterminate)
		{
			const float t = TimeCodex::Get().GetTotalTime();
			const float phase = t - std::floor(t);
			const unsigned chunk = std::max(1u, innerW / 4u);
			const unsigned travel = (innerW > chunk) ? innerW - chunk : 0u;
			const unsigned xStart = innerLeft + static_cast<unsigned>(phase * static_cast<float>(travel));
			FillRect(c, xStart, innerTop, xStart + chunk - 1u, innerBottom, style_.indeterminateColor);
			return;
		}

		const unsigned fillW = static_cast<unsigned>(std::round(vm.normalized * static_cast<float>(innerW)));
		if (fillW == 0u)
			return;

		FillRect(c, innerLeft, innerTop, innerLeft + fillW - 1u, innerBottom, fill);
	}

	void ProgressBarCanvasView::SyncFrom(const ProgressBarViewModel& vm)
	{
		const bool dirty = !hasPainted_
			|| vm.normalized != lastPainted_.normalized
			|| vm.indeterminate != lastPainted_.indeterminate
			|| vm.enabled != lastPainted_.enabled;

		if (vm.indeterminate)
		{
			Repaint_(vm);
			lastPainted_ = vm;
			hasPainted_ = true;
			return;
		}

		if (!dirty)
			return;

		Repaint_(vm);
		lastPainted_ = vm;
		hasPainted_ = true;
	}
}
