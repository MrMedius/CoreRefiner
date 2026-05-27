#include "DropdownListItemCanvasView.h"

#include "Canvas.h"
#include "Graphics.h"
#include "TextCodex.h"

#include <algorithm>

namespace Ui
{
	namespace
	{
		constexpr unsigned kMaxCanvasPixelDim = 2048u;

		[[nodiscard]] unsigned ClampCanvasPixelDim(const unsigned value) noexcept
		{
			return std::max(1u, std::min(value, kMaxCanvasPixelDim));
		}

		void DrawFocusRing(::Canvas& c, const Color ring, const unsigned thick)
		{
			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			if (w == 0u || h == 0u || thick == 0u)
				return;

			for (unsigned t = 0; t < thick; ++t)
			{
				if (t >= w || t >= h)
					break;
				const unsigned y1 = t;
				const unsigned y2 = h - 1u - t;
				for (unsigned x = 0; x < w; ++x)
				{
					c.PutPixel(x, y1, ring);
					if (y2 != y1)
						c.PutPixel(x, y2, ring);
				}
				const unsigned x1 = t;
				const unsigned x2 = w - 1u - t;
				for (unsigned y = y1; y <= y2; ++y)
				{
					c.PutPixel(x1, y, ring);
					if (x2 != x1)
						c.PutPixel(x2, y, ring);
				}
			}
		}
	}

	DropdownListItemCanvasView::DropdownListItemCanvasView(
		Graphics& gfx,
		const unsigned pixelWidth,
		const unsigned pixelHeight,
		const DropdownCanvasStyle& style)
		:
		style_(style),
		pixelWidth_(ClampCanvasPixelDim(pixelWidth)),
		pixelHeight_(ClampCanvasPixelDim(pixelHeight)),
		bgCanvas_(std::make_unique<Canvas2D>(gfx, 1u, 1u)),
		ringCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth_, pixelHeight_)),
		textCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth_, pixelHeight_))
	{
		ringCanvas_->Clear(Colors::None);
		textCanvas_->Clear(Colors::None);
	}

	void DropdownListItemCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		bgCanvas_->LinkTechniques(rg);
		ringCanvas_->LinkTechniques(rg);
		textCanvas_->LinkTechniques(rg);
	}

	void DropdownListItemCanvasView::Submit(const std::size_t channelMask) const
	{
		bgCanvas_->Submit(channelMask);
		ringCanvas_->Submit(channelMask);
		textCanvas_->Submit(channelMask);
	}

	Color DropdownListItemCanvasView::BackgroundForRow_(const DropdownListItemViewModel& vm) const noexcept
	{
		if (vm.phase == UiVisualPhase::Pressed)
			return style_.itemPressed;
		if (vm.phase == UiVisualPhase::Focused)
		{
			if (vm.highlighted)
				return style_.itemHighlight;
			if (vm.selected)
				return style_.itemSelected;
		}
		if (vm.selected)
			return style_.itemSelected;
		return style_.itemNormal;
	}

	void DropdownListItemCanvasView::ApplyLayout(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		const float safeW = std::max(1.0f, width);
		const float safeH = std::max(1.0f, height);
		const DirectX::XMFLOAT3 pos{ centerX, centerY, 0.0f };
		const DirectX::XMFLOAT3 scale{ safeW, safeH, 1.0f };
		bgCanvas_->SetPosition(pos);
		bgCanvas_->SetScale(scale);
		ringCanvas_->SetPosition(pos);
		ringCanvas_->SetScale(scale);
		textCanvas_->SetPosition(pos);
		textCanvas_->SetScale(scale);
	}

	void DropdownListItemCanvasView::RepaintBackground_(const DropdownListItemViewModel& vm)
	{
		::Canvas& c = *bgCanvas_;
		c.Clear(BackgroundForRow_(vm));
	}

	void DropdownListItemCanvasView::RepaintFocusRing_(const DropdownListItemViewModel& vm)
	{
		::Canvas& c = *ringCanvas_;
		c.Clear(Colors::None);
		if (vm.phase == UiVisualPhase::Focused)
			DrawFocusRing(c, style_.itemFocusRing, style_.itemFocusRingThicknessPx);
	}

	void DropdownListItemCanvasView::RepaintText_(const DropdownListItemViewModel& vm)
	{
		::Canvas& c = *textCanvas_;
		c.Clear(Colors::None);

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = vm.label.empty() ? " " : vm.label;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = style_.fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = style_.headerPaddingPx;
		rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
		rq.defaultColor = style_.itemText;
		rq.backgroundColor = Colors::None;
		ctx.Render(c);
	}

	void DropdownListItemCanvasView::SyncFrom(const DropdownListItemViewModel& vm)
	{
		const bool bgDirty = !hasPainted_
			|| vm.phase != lastPainted_.phase
			|| vm.highlighted != lastPainted_.highlighted
			|| vm.selected != lastPainted_.selected;

		const bool ringDirty = !hasPainted_
			|| vm.phase != lastPainted_.phase;

		const bool textDirty = !hasPainted_
			|| vm.label != lastPainted_.label;

		if (bgDirty)
			RepaintBackground_(vm);

		if (ringDirty)
			RepaintFocusRing_(vm);

		if (textDirty)
			RepaintText_(vm);

		if (bgDirty || ringDirty || textDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
