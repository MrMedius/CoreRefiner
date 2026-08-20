#include "ToggleCanvasView.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	namespace
	{
		void DrawThickLine(Canvas& c, float x0, float y0, float x1, float y1, Color col, unsigned thickness)
		{
			const float dx = x1 - x0;
			const float dy = y1 - y0;
			const float len = std::sqrt(dx * dx + dy * dy);
			if (len <= 0.0f)
				return;

			const float halfT = static_cast<float>(thickness) * 0.5f;
			const unsigned steps = static_cast<unsigned>(std::max(1.0f, len * 2.0f));

			for (unsigned i = 0u; i <= steps; ++i)
			{
				const float t = static_cast<float>(i) / static_cast<float>(steps);
				const float px = x0 + dx * t;
				const float py = y0 + dy * t;

				for (int oy = -static_cast<int>(halfT); oy <= static_cast<int>(halfT); ++oy)
					for (int ox = -static_cast<int>(halfT); ox <= static_cast<int>(halfT); ++ox)
						c.PutPixel(static_cast<unsigned>(px + static_cast<float>(ox)), static_cast<unsigned>(py + static_cast<float>(oy)), col);
			}
		}

		void ComputeBoxOuterRect(
			const unsigned side,
			const float boxScale,
			unsigned& x0,
			unsigned& y0,
			unsigned& x1,
			unsigned& y1) noexcept
		{
			const float clamped = std::clamp(boxScale, 0.01f, 1.0f);
			const unsigned inner = std::max(2u, static_cast<unsigned>(std::lround(static_cast<double>(side) * static_cast<double>(clamped))));
			const unsigned offset = (side - inner) / 2u;
			x0 = offset;
			y0 = offset;
			x1 = offset + inner - 1u;
			y1 = offset + inner - 1u;
		}
	}

	ToggleCanvasView::ToggleCanvasView(Graphics& gfx, const unsigned pixelSize, ToggleCanvasStyle style)
		:
		style_(std::move(style)),
		boxCanvas_(std::make_unique<Canvas2D>(gfx, pixelSize, pixelSize)),
		checkCanvas_(std::make_unique<Canvas2D>(gfx, std::max(1u, pixelSize), std::max(1u, pixelSize)))
	{
		BakeCheckGeometry_();
	}

	void ToggleCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		boxCanvas_->LinkTechniques(rg);
		checkCanvas_->LinkTechniques(rg);
	}

	void ToggleCanvasView::Submit(const std::size_t channelMask) const
	{
		boxCanvas_->Submit(channelMask);
		if (checkVisible_)
			checkCanvas_->Submit(channelMask);
	}

	Color ToggleCanvasView::BoxColorForPhase(const ToggleViewModel& vm) const noexcept
	{
		if (!vm.enabled)
			return style_.boxDisabled;
		switch (vm.phase)
		{
		case UiVisualPhase::Disabled: return style_.boxDisabled;
		case UiVisualPhase::Pressed:  return style_.boxPressed;
		case UiVisualPhase::Focused:  return style_.boxFocused;
		default:                      return style_.boxNormal;
		}
	}

	Color ToggleCanvasView::CheckColorForPhase(const ToggleViewModel& vm) const noexcept
	{
		return vm.enabled ? style_.checkColor : style_.checkDisabledColor;
	}

	void ToggleCanvasView::BakeCheckGeometry_()
	{
		Canvas& c = *checkCanvas_;
		const unsigned side = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (side == 0u || h == 0u)
			return;

		c.Clear(Colors::None);

		const unsigned inset = style_.checkInsetPx;
		if (inset * 2u >= side || inset * 2u >= h)
			return;

		const unsigned thickness = std::max(2u, side / 12u);

		const float x0 = static_cast<float>(inset);
		const float y0 = static_cast<float>(h) * 0.55f;
		const float x1 = static_cast<float>(side) * 0.38f;
		const float y1 = static_cast<float>(h) - static_cast<float>(inset) - 1.0f;
		const float x2 = static_cast<float>(side) - static_cast<float>(inset) - 1.0f;
		const float y2 = static_cast<float>(inset);

		DrawThickLine(c, x0, y0, x1, y1, style_.checkColor, thickness);
		DrawThickLine(c, x1, y1, x2, y2, style_.checkColor, thickness);
	}

	void ToggleCanvasView::RepaintBox_(const ToggleViewModel& vm)
	{
		Canvas& c = *boxCanvas_;
		const unsigned side = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (side == 0u || h == 0u)
			return;

		c.Clear(Colors::None);

		unsigned outerX0 = 0u;
		unsigned outerY0 = 0u;
		unsigned outerX1 = side - 1u;
		unsigned outerY1 = h - 1u;
		if (layoutBoxScale_ < 1.0f)
			ComputeBoxOuterRect(side, layoutBoxScale_, outerX0, outerY0, outerX1, outerY1);

		const Color boxFill = BoxColorForPhase(vm);
		const unsigned border = style_.borderPx;

		if (border > 0u
			&& border * 2u < (outerX1 - outerX0 + 1u)
			&& border * 2u < (outerY1 - outerY0 + 1u))
		{
			CanvasPixelDraw::FillRect(
				c,
				outerX0 + border,
				outerY0 + border,
				outerX1 - border,
				outerY1 - border,
				boxFill);
			CanvasPixelDraw::DrawRectBorder(c, outerX0, outerY0, outerX1, outerY1, border, style_.borderColor);
			return;
		}

		CanvasPixelDraw::FillRect(c, outerX0, outerY0, outerX1, outerY1, boxFill);
	}

	void ToggleCanvasView::RepaintCheckColor_(const ToggleViewModel& vm)
	{
		if (!vm.isOn)
			return;

		Canvas& c = *checkCanvas_;
		const unsigned side = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (side == 0u || h == 0u)
			return;

		c.Clear(Colors::None);

		const unsigned inset = style_.checkInsetPx;
		if (inset * 2u >= side || inset * 2u >= h)
			return;

		const Color check = CheckColorForPhase(vm);
		const unsigned thickness = std::max(2u, side / 12u);
		const float x0 = static_cast<float>(inset);
		const float y0 = static_cast<float>(h) * 0.55f;
		const float x1 = static_cast<float>(side) * 0.38f;
		const float y1 = static_cast<float>(h) - static_cast<float>(inset) - 1.0f;
		const float x2 = static_cast<float>(side) - static_cast<float>(inset) - 1.0f;
		const float y2 = static_cast<float>(inset);

		DrawThickLine(c, x0, y0, x1, y1, check, thickness);
		DrawThickLine(c, x1, y1, x2, y2, check, thickness);
	}

	void ToggleCanvasView::ApplyLayout(
		const float centerX,
		const float centerY,
		const float size,
		const float boxScale) noexcept
	{
		if (layoutBoxScale_ != boxScale)
		{
			layoutBoxScale_ = boxScale;
			boxLayoutDirty_ = true;
		}

		const DirectX::XMFLOAT3 pos{ centerX, centerY, 0.0f };
		const DirectX::XMFLOAT3 scale{ size, size, 1.0f };
		boxCanvas_->SetPosition(pos);
		boxCanvas_->SetScale(scale);
		checkCanvas_->SetPosition(pos);
		checkCanvas_->SetScale(scale);
	}

	void ToggleCanvasView::SyncFrom(const ToggleViewModel& vm)
	{
		const bool boxDirty = !hasPainted_
			|| boxLayoutDirty_
			|| vm.phase != lastPainted_.phase
			|| vm.enabled != lastPainted_.enabled;

		const bool checkColorDirty = !hasPainted_
			|| vm.enabled != lastPainted_.enabled;

		const bool checkVisibleDirty = !hasPainted_
			|| vm.isOn != lastPainted_.isOn;

		const bool checkPaintDirty = checkColorDirty || (checkVisibleDirty && vm.isOn);

		if (boxDirty)
		{
			RepaintBox_(vm);
			boxLayoutDirty_ = false;
		}

		if (checkPaintDirty)
			RepaintCheckColor_(vm);

		if (checkVisibleDirty)
			checkVisible_ = vm.isOn;

		if (boxDirty || checkPaintDirty || checkVisibleDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
