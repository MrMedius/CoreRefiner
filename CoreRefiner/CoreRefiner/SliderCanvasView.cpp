#include "SliderCanvasView.h"

#include "Channels.h"

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

	SliderCanvasView::SliderCanvasView(
		Graphics& gfx,
		const unsigned pixelWidth,
		const unsigned pixelHeight,
		SliderCanvasStyle style)
		:
		style_(std::move(style)),
		trackCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight)),
		fillCanvas_(std::make_unique<Canvas2D>(gfx, std::max(1u, pixelWidth), std::max(1u, pixelHeight)))
	{}

	void SliderCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		trackCanvas_->LinkTechniques(rg);
		fillCanvas_->LinkTechniques(rg);
	}

	void SliderCanvasView::Submit(const std::size_t channelMask) const
	{
		trackCanvas_->Submit(channelMask);
		fillCanvas_->Submit(channelMask);
	}

	Color SliderCanvasView::TrackColorForPhase(const SliderViewModel& vm) const noexcept
	{
		if (!vm.enabled)
			return style_.trackDisabled;
		switch (vm.phase)
		{
		case UiVisualPhase::Disabled: return style_.trackDisabled;
		case UiVisualPhase::Pressed:  return style_.trackPressed;
		case UiVisualPhase::Focused:  return style_.trackFocused;
		default:                     return style_.trackNormal;
		}
	}

	Color SliderCanvasView::FillColorForPhase(const SliderViewModel& vm) const noexcept
	{
		return vm.enabled ? style_.fillNormal : style_.fillDisabled;
	}

	void SliderCanvasView::RepaintTrack_(const SliderViewModel& vm)
	{
		Canvas& c = *trackCanvas_;
		const unsigned w = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (w == 0u || h == 0u)
			return;

		const Color track = TrackColorForPhase(vm);
		c.Clear(track);

		const unsigned border = style_.borderPx;
		if (border > 0u && border * 2u < w && border * 2u < h)
		{
			const unsigned y0 = border;
			const unsigned y1 = h - 1u - border;
			FillRect(c, 0u, 0u, w - 1u, border - 1u, style_.borderColor);
			FillRect(c, 0u, h - border, w - 1u, h - 1u, style_.borderColor);
			FillRect(c, 0u, y0, border - 1u, y1, style_.borderColor);
			FillRect(c, w - border, y0, w - 1u, y1, style_.borderColor);
		}

		if (!fillInitialized_)
		{
			fillCanvas_->Clear(FillColorForPhase(vm));
			fillInitialized_ = true;
		}
	}

	void SliderCanvasView::ApplyFillTransform_(const SliderViewModel& vm)
	{
		const float pad = static_cast<float>(style_.paddingPx + style_.borderPx);
		const float innerW = std::max(0.0f, vm.layoutWidth - 2.0f * pad);
		const float innerH = std::max(0.0f, vm.layoutHeight - 2.0f * pad);
		const float minExtent = 1.0f;

		if (vm.axis == SliderAxis::Horizontal)
		{
			const float fillW = std::max(minExtent, vm.normalized * innerW);
			const float cx = vm.layoutCenterX - vm.layoutWidth * 0.5f + pad + fillW * 0.5f;
			fillCanvas_->SetPosition(DirectX::XMFLOAT3{ cx, vm.layoutCenterY, 0.0f });
			fillCanvas_->SetScale(DirectX::XMFLOAT3{ fillW, vm.layoutHeight - 2.0f * pad, 1.0f });
		}
		else
		{
			const float fillH = std::max(minExtent, vm.normalized * innerH);
			const float cy = vm.layoutCenterY - vm.layoutHeight * 0.5f + pad + fillH * 0.5f;
			fillCanvas_->SetPosition(DirectX::XMFLOAT3{ vm.layoutCenterX, cy, 0.0f });
			fillCanvas_->SetScale(DirectX::XMFLOAT3{ vm.layoutWidth - 2.0f * pad, fillH, 1.0f });
		}
	}

	void SliderCanvasView::SyncFrom(const SliderViewModel& vm)
	{
		const bool trackDirty = !hasPainted_
			|| vm.phase != lastPainted_.phase
			|| vm.enabled != lastPainted_.enabled
			|| vm.interactive != lastPainted_.interactive;

		const bool fillDirty = !hasPainted_
			|| vm.normalized != lastPainted_.normalized
			|| vm.layoutWidth != lastPainted_.layoutWidth
			|| vm.layoutHeight != lastPainted_.layoutHeight
			|| vm.layoutCenterX != lastPainted_.layoutCenterX
			|| vm.layoutCenterY != lastPainted_.layoutCenterY
			|| vm.axis != lastPainted_.axis;

		if (trackDirty)
		{
			RepaintTrack_(vm);
			fillCanvas_->Clear(FillColorForPhase(vm));
			fillInitialized_ = true;
		}

		if (fillDirty || trackDirty)
			ApplyFillTransform_(vm);

		if (trackDirty || fillDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
