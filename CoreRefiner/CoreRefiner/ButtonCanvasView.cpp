#include "ButtonCanvasView.h"

#include "Channels.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "TextCodex.h"

namespace Ui
{
	namespace
	{
		Color BackgroundForPhase(const ButtonCanvasStyle& s, ButtonVisualPhase phase)
		{
			switch (phase)
			{
			case ButtonVisualPhase::Disabled: return s.bgDisabled;
			case ButtonVisualPhase::Pressed:  return s.bgPressed;
			case ButtonVisualPhase::Hovered:  return s.bgHovered;
			case ButtonVisualPhase::Focused:  return s.bgFocused;
			default:                         return s.bgNormal;
			}
		}

		Color TextColorForPhase(const ButtonCanvasStyle& s, ButtonVisualPhase phase)
		{
			if (phase == ButtonVisualPhase::Disabled)
				return s.textDisabled;
			return s.textNormal;
		}

		void DrawFocusRing(Canvas& c, Color ring, unsigned thick)
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

	ButtonCanvasView::ButtonCanvasView(Graphics& gfx, unsigned pixelWidth, unsigned pixelHeight, ButtonCanvasStyle style)
		:
		style_(std::move(style)),
		canvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight))
	{}

	void ButtonCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		canvas_->LinkTechniques(rg);
	}

	void ButtonCanvasView::Submit(std::size_t channelMask) const
	{
		canvas_->Submit(channelMask);
	}

	void ButtonCanvasView::Repaint_(const ButtonViewModel& vm)
	{
		Canvas& c = *canvas_;
		const Color fill = BackgroundForPhase(style_, vm.phase);
		c.Clear(fill);

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.utf8Text = vm.labelUtf8;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = style_.fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
		rq.style.wordWrapEnabled = true;
		rq.paddingPx = style_.paddingPx;
		rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
		rq.defaultColor = TextColorForPhase(style_, vm.phase);
		rq.backgroundColor = Colors::None;

		ctx.Render(c);
	
		if (vm.phase == ButtonVisualPhase::Focused)
			DrawFocusRing(c, style_.focusRingColor, style_.focusRingThicknessPx);
	}

	void ButtonCanvasView::SyncFrom(const ButtonViewModel& vm)
	{
		const bool dirty = !hasPainted_
			|| vm.phase != lastPainted_.phase
			|| vm.labelUtf8 != lastPainted_.labelUtf8;

		if (!dirty)
			return;

		Repaint_(vm);
		lastPainted_ = vm;
		hasPainted_ = true;
	}
}