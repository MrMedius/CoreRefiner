#include "ToggleCanvasView.h"
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
	}

	ToggleCanvasView::ToggleCanvasView(Graphics& gfx, const unsigned pixelSize, ToggleCanvasStyle style)
		:
		style_(std::move(style)),
		boxCanvas_(std::make_unique<Canvas2D>(gfx, pixelSize, pixelSize)),
		checkCanvas_(std::make_unique<Canvas2D>(gfx, std::max(1u, pixelSize), std::max(1u, pixelSize)))
	{}

	void ToggleCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		boxCanvas_->LinkTechniques(rg);
		checkCanvas_->LinkTechniques(rg);
	}

	void ToggleCanvasView::Submit(const std::size_t channelMask) const
	{
		boxCanvas_->Submit(channelMask);
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
		default:                     return style_.boxNormal;
		}
	}

	Color ToggleCanvasView::CheckColorForPhase(const ToggleViewModel& vm) const noexcept
	{
		return vm.enabled ? style_.checkColor : style_.checkDisabledColor;
	}

	void ToggleCanvasView::RepaintBox_(const ToggleViewModel& vm)
	{
		Canvas& c = *boxCanvas_;
		const unsigned side = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (side == 0u || h == 0u)
			return;

		const Color boxFill = BoxColorForPhase(vm);
		c.Clear(boxFill);

		const unsigned border = style_.borderPx;
		if (border > 0u && border * 2u < side && border * 2u < h)
		{
			FillRect(c, 0u, 0u, side - 1u, border - 1u, style_.borderColor);
			FillRect(c, 0u, h - border, side - 1u, h - 1u, style_.borderColor);
			FillRect(c, 0u, border, border - 1u, h - 1u - border, style_.borderColor);
			FillRect(c, side - border, border, side - 1u, h - 1u - border, style_.borderColor);
		}
	}

	void ToggleCanvasView::RepaintCheck_(const ToggleViewModel& vm)
	{
		Canvas& c = *checkCanvas_;
		const unsigned side = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (side == 0u || h == 0u)
			return;

		c.Clear(Colors::None);
		if (!vm.isOn)
			return;

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

	void ToggleCanvasView::ApplyLayout_(const ToggleViewModel& vm)
	{
		const DirectX::XMFLOAT3 pos{ vm.layoutCenterX, vm.layoutCenterY, 0.0f };
		const DirectX::XMFLOAT3 scale{ vm.layoutSize, vm.layoutSize, 1.0f };
		boxCanvas_->SetPosition(pos);
		boxCanvas_->SetScale(scale);
		checkCanvas_->SetPosition(pos);
		checkCanvas_->SetScale(scale);
	}

	void ToggleCanvasView::SyncFrom(const ToggleViewModel& vm)
	{
		const bool boxDirty = !hasPainted_
			|| vm.phase != lastPainted_.phase
			|| vm.enabled != lastPainted_.enabled;

		const bool checkDirty = !hasPainted_
			|| vm.isOn != lastPainted_.isOn
			|| vm.enabled != lastPainted_.enabled;

		const bool layoutDirty = !hasPainted_
			|| vm.layoutCenterX != lastPainted_.layoutCenterX
			|| vm.layoutCenterY != lastPainted_.layoutCenterY
			|| vm.layoutSize != lastPainted_.layoutSize;

		if (layoutDirty)
			ApplyLayout_(vm);

		if (boxDirty)
			RepaintBox_(vm);

		if (checkDirty)
			RepaintCheck_(vm);

		if (boxDirty || checkDirty || layoutDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
