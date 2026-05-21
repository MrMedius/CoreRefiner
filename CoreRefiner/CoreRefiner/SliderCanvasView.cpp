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
		fillCanvas_(std::make_unique<SliderCanvasFill>(gfx, std::max(1u, pixelWidth), std::max(1u, pixelHeight)))
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
	}

	void SliderCanvasView::ApplyTrackLayout_(const SliderViewModel& vm)
	{
		trackCanvas_->SetPosition(DirectX::XMFLOAT3{ vm.outerCenterX, vm.outerCenterY, 0.0f });
		trackCanvas_->SetScale(DirectX::XMFLOAT3{ vm.outerWidth, vm.outerHeight, 1.0f });
		trackCanvas_->SetRotation(0.0f, 0.0f, vm.rotationDegZ);
	}

	void SliderCanvasView::ApplyFillLayout_(const SliderViewModel& vm)
	{
		fillCanvas_->SetPosition(DirectX::XMFLOAT3{ vm.grooveCenterX, vm.grooveCenterY, 0.0f });
		fillCanvas_->SetScale(DirectX::XMFLOAT3{ vm.grooveWidth, vm.grooveHeight, 1.0f });
		fillCanvas_->SetRotation(0.0f, 0.0f, vm.rotationDegZ);
	}

	void SliderCanvasView::ApplyFillParams_(const SliderViewModel& vm)
	{
		SliderCanvasFill::Params params{};
		params.fillAmount = vm.normalized;
		fillCanvas_->SetParams(params);
	}

	void SliderCanvasView::SyncFrom(const SliderViewModel& vm)
	{
		const bool trackDirty = !hasPainted_
			|| vm.phase != lastPainted_.phase
			|| vm.enabled != lastPainted_.enabled
			|| vm.interactive != lastPainted_.interactive;

		const bool layoutDirty = !hasPainted_
			|| vm.outerCenterX != lastPainted_.outerCenterX
			|| vm.outerCenterY != lastPainted_.outerCenterY
			|| vm.outerWidth != lastPainted_.outerWidth
			|| vm.outerHeight != lastPainted_.outerHeight
			|| vm.grooveCenterX != lastPainted_.grooveCenterX
			|| vm.grooveCenterY != lastPainted_.grooveCenterY
			|| vm.grooveWidth != lastPainted_.grooveWidth
			|| vm.grooveHeight != lastPainted_.grooveHeight
			|| vm.rotationDegZ != lastPainted_.rotationDegZ;

		const bool fillParamsDirty = !hasPainted_
			|| vm.normalized != lastPainted_.normalized;

		if (trackDirty)
		{
			RepaintTrack_(vm);
			fillCanvas_->Clear(FillColorForPhase(vm));
		}

		if (layoutDirty)
		{
			ApplyTrackLayout_(vm);
			ApplyFillLayout_(vm);
		}

		if (fillParamsDirty || layoutDirty)
			ApplyFillParams_(vm);

		if (trackDirty || layoutDirty || fillParamsDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}