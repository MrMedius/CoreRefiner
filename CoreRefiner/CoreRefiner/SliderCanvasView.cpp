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

		[[nodiscard]] float RadToDeg(const float rad) noexcept
		{
			return rad * (180.0f / 3.14159265358979323846f);
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
	{
		BakeTrackBorder_();
	}

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

	void SliderCanvasView::BakeTrackBorder_()
	{
		Canvas& c = *trackCanvas_;
		const unsigned w = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (w == 0u || h == 0u)
			return;

		c.Clear(Colors::None);

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

	void SliderCanvasView::RepaintTrackFill_(const SliderViewModel& vm)
	{
		Canvas& c = *trackCanvas_;
		const unsigned w = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (w == 0u || h == 0u)
			return;

		const unsigned border = style_.borderPx;
		const Color track = TrackColorForPhase(vm);

		if (border > 0u && border * 2u < w && border * 2u < h)
		{
			const unsigned y0 = border;
			const unsigned y1 = h - 1u - border;
			FillRect(c, border, y0, w - 1u - border, y1, track);
			return;
		}

		c.Clear(track);
	}

	void SliderCanvasView::ApplyLayout(const SliderGrooveLayout& layout) noexcept
	{
		const float rotationDeg = RadToDeg(layout.rotationRadZ);
		trackCanvas_->SetPosition(DirectX::XMFLOAT3{ layout.centerX, layout.centerY, 0.0f });
		trackCanvas_->SetScale(DirectX::XMFLOAT3{ layout.outerWidth, layout.outerHeight, 1.0f });
		trackCanvas_->SetRotation(0.0f, 0.0f, rotationDeg);

		fillCanvas_->SetPosition(DirectX::XMFLOAT3{ layout.centerX, layout.centerY, 0.0f });
		fillCanvas_->SetScale(DirectX::XMFLOAT3{ layout.grooveWidth, layout.grooveHeight, 1.0f });
		fillCanvas_->SetRotation(0.0f, 0.0f, rotationDeg);
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

		const bool fillParamsDirty = !hasPainted_
			|| vm.normalized != lastPainted_.normalized;

		if (trackDirty)
		{
			RepaintTrackFill_(vm);
			fillCanvas_->Clear(FillColorForPhase(vm));
		}

		if (fillParamsDirty)
			ApplyFillParams_(vm);

		if (trackDirty || fillParamsDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}