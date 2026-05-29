#include "StepperCanvasView.h"

#include "Channels.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "TextCodex.h"

#include <algorithm>

namespace Ui
{
	namespace
	{
		/** @brief 在 Canvas 内壁绘制矩形焦点环。 */
		void DrawFocusRing(Canvas& c, const Color ring, const unsigned thick)
		{
			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			if (w == 0u || h == 0u || thick == 0u)
				return;
			for (unsigned t = 0u; t < thick; ++t)
			{
				if (t >= w || t >= h)
					break;
				const unsigned y1 = t;
				const unsigned y2 = h - 1u - t;
				for (unsigned x = 0u; x < w; ++x)
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

		void TintWhiteShapePixels(Canvas& c, const Color color)
		{
			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			for (unsigned y = 0u; y < h; ++y)
			{
				for (unsigned x = 0u; x < w; ++x)
				{
					const Color px = c.GetPixel(x, y);
					if (px.GetA() > 0u)
						c.PutPixel(x, y, color);
				}
			}
		}

		void RenderTextOnCanvas(
			Canvas& c,
			const std::string& text,
			const Color bgColor,
			const Color textColor,
			const Text::FontSource& font,
			const float fontSize,
			const int paddingPx)
		{
			c.Clear(bgColor);

			auto ctx = TextCodex::Get().BeginDraw();
			Text::RenderRequest& rq = ctx.Request();
			rq.text = text;
			rq.canvasMode = Text::CanvasMode::Fixed;
			rq.clearMode = Text::ClearMode::NoClear;
			rq.primaryFont = font;
			rq.style.fontSize = fontSize;
			rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
			rq.style.wordWrapEnabled = true;
			rq.paddingPx = paddingPx;
			rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
			rq.defaultColor = textColor;
			rq.backgroundColor = Colors::None;
			ctx.Render(c);
		}
	}

	StepperCanvasView::StepperCanvasView(
		Graphics& gfx,
		const unsigned btnPixelSize,
		const unsigned textPixelW,
		const unsigned pixelH,
		StepperCanvasStyle style)
		:
		style_(std::move(style)),
		minusBgCanvas_(std::make_unique<Canvas2D>(gfx, btnPixelSize, pixelH)),
		minusArrowCanvas_(std::make_unique<Canvas2D>(gfx, btnPixelSize, pixelH)),
		textCanvas_(std::make_unique<Canvas2D>(gfx, textPixelW, pixelH)),
		plusBgCanvas_(std::make_unique<Canvas2D>(gfx, btnPixelSize, pixelH)),
		plusArrowCanvas_(std::make_unique<Canvas2D>(gfx, btnPixelSize, pixelH)),
		ringCanvas_(std::make_unique<Canvas2D>(gfx, btnPixelSize * 2u + textPixelW, pixelH))
	{
		minusArrowCanvas_->ApplyForm(Canvas::Form::Triangle, 0.0f);
		minusArrowCanvas_->SetRotation(0.0f, 0.0f, -90.0f);
		TintWhiteShapePixels(*minusArrowCanvas_, style_.symbolNormal);

		plusArrowCanvas_->ApplyForm(Canvas::Form::Triangle, 0.0f);
		plusArrowCanvas_->SetRotation(0.0f, 0.0f, 90.0f);
		TintWhiteShapePixels(*plusArrowCanvas_, style_.symbolNormal);

		ringCanvas_->Clear(Colors::None);
	}

	void StepperCanvasView::ApplyLayout(
		const float totalCenterX,
		const float totalCenterY,
		const float buttonWidth,
		const float textWidth,
		const float centerGapWidth,
		const float height,
		const bool showCenterText) noexcept
	{
		showCenterText_ = showCenterText;

		const float halfBtn = buttonWidth * 0.5f;
		const float middleSpan = showCenterText ? textWidth : centerGapWidth;
		const float halfMiddle = middleSpan * 0.5f;

		const DirectX::XMFLOAT3 btnScale{ buttonWidth, height, 1.0f };

		const float minusCx = totalCenterX - halfMiddle - halfBtn;
		const float plusCx  = totalCenterX + halfMiddle + halfBtn;

		minusBgCanvas_->SetPosition({ minusCx, totalCenterY, 0.0f });
		minusBgCanvas_->SetScale(btnScale);
		minusArrowCanvas_->SetPosition({ minusCx, totalCenterY, 0.0f });
		minusArrowCanvas_->SetScale({ btnScale.x * 0.8f ,btnScale.y * 0.8f ,btnScale .z});

		if (showCenterText)
		{
			textCanvas_->SetPosition({ totalCenterX, totalCenterY, 0.0f });
			textCanvas_->SetScale({ textWidth, height, 1.0f });
		}

		plusBgCanvas_->SetPosition({ plusCx, totalCenterY, 0.0f });
		plusBgCanvas_->SetScale(btnScale);
		plusArrowCanvas_->SetPosition({ plusCx, totalCenterY, 0.0f });
		plusArrowCanvas_->SetScale({ btnScale.x * 0.8f ,btnScale.y * 0.8f ,btnScale.z });

		const float ringWidth = buttonWidth * 2.0f + middleSpan;
		const auto ringPixelW = static_cast<unsigned>(std::max(1.0f, ringWidth));
		const auto ringPixelH = static_cast<unsigned>(std::max(1.0f, height));
		if (ringCanvas_->GetCanvasWidth() != ringPixelW || ringCanvas_->GetCanvasHeight() != ringPixelH)
			ringCanvas_->Resize(ringPixelW, ringPixelH);
		ringCanvas_->SetPosition({ totalCenterX, totalCenterY, 0.0f });
		ringCanvas_->SetScale({ ringWidth, height, 1.0f });
	}

	Color StepperCanvasView::BtnBgForPhase(const UiVisualPhase phase) const noexcept
	{
		switch (phase)
		{
		case UiVisualPhase::Disabled: return style_.btnBgDisabled;
		case UiVisualPhase::Pressed:  return style_.btnBgPressed;
		case UiVisualPhase::Focused:  return style_.btnBgFocused;
		default:                      return style_.btnBgNormal;
		}
	}

	Color StepperCanvasView::SymbolColorForEnabled(const bool enabled) const noexcept
	{
		return enabled ? style_.symbolNormal : style_.symbolDisabled;
	}

	Color StepperCanvasView::TextColorForEnabled(const bool enabled) const noexcept
	{
		return enabled ? style_.textNormal : style_.textDisabled;
	}

	void StepperCanvasView::RepaintMinusBg_(const UiVisualPhase phase)
	{
		minusBgCanvas_->Clear(BtnBgForPhase(phase));
	}

	void StepperCanvasView::RepaintPlusBg_(const UiVisualPhase phase)
	{
		plusBgCanvas_->Clear(BtnBgForPhase(phase));
	}

	void StepperCanvasView::RepaintMinusArrow_(const bool enabled)
	{
		minusArrowCanvas_->ReapplyForm();
		TintWhiteShapePixels(*minusArrowCanvas_, SymbolColorForEnabled(enabled));
	}

	void StepperCanvasView::RepaintPlusArrow_(const bool enabled)
	{
		plusArrowCanvas_->ReapplyForm();
		TintWhiteShapePixels(*plusArrowCanvas_, SymbolColorForEnabled(enabled));
	}

	void StepperCanvasView::RepaintText_(const StepperViewModel& vm)
	{
		const Color bg   = vm.enabled ? style_.textBgNormal : style_.textBgDisabled;
		const Color text = TextColorForEnabled(vm.enabled);

		RenderTextOnCanvas(
			*textCanvas_,
			vm.displayText,
			bg,
			text,
			style_.primaryFont,
			style_.fontSize,
			style_.paddingPx);
	}

	void StepperCanvasView::RepaintRing_(const bool show)
	{
		Canvas& c = *ringCanvas_;
		c.Clear(Colors::None);
		if (show)
			DrawFocusRing(c, style_.focusRingColor, style_.focusRingThicknessPx);
	}

	void StepperCanvasView::SyncFrom(const StepperViewModel& vm)
	{
		const bool minusBgDirty = !hasPainted_
			|| vm.minusPhase != lastPainted_.minusPhase;

		const bool arrowDirty = !hasPainted_
			|| vm.enabled != lastPainted_.enabled;

		const bool plusBgDirty = !hasPainted_
			|| vm.plusPhase != lastPainted_.plusPhase;

		const bool textDirty = vm.showCenterText
			&& (!hasPainted_
				|| vm.displayText != lastPainted_.displayText
				|| vm.enabled != lastPainted_.enabled
				|| vm.showCenterText != lastPainted_.showCenterText);

		const bool ringDirty = !hasPainted_
			|| vm.showFocusRing != lastPainted_.showFocusRing
			|| vm.showCenterText != lastPainted_.showCenterText
			|| vm.centerGapWidth != lastPainted_.centerGapWidth;

		if (minusBgDirty)
			RepaintMinusBg_(vm.minusPhase);

		if (plusBgDirty)
			RepaintPlusBg_(vm.plusPhase);

		if (arrowDirty)
		{
			RepaintMinusArrow_(vm.enabled);
			RepaintPlusArrow_(vm.enabled);
		}

		if (textDirty)
			RepaintText_(vm);

		if (ringDirty)
			RepaintRing_(vm.showFocusRing);

		if (minusBgDirty || arrowDirty || plusBgDirty || textDirty || ringDirty)
		{
			lastPainted_ = vm;
			hasPainted_  = true;
		}
	}

	void StepperCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		minusBgCanvas_->LinkTechniques(rg);
		minusArrowCanvas_->LinkTechniques(rg);
		textCanvas_->LinkTechniques(rg);
		plusBgCanvas_->LinkTechniques(rg);
		plusArrowCanvas_->LinkTechniques(rg);
		ringCanvas_->LinkTechniques(rg);
	}

	void StepperCanvasView::Submit(const std::size_t channelMask) const
	{
		minusBgCanvas_->Submit(channelMask);
		minusArrowCanvas_->Submit(channelMask);
		if (showCenterText_)
			textCanvas_->Submit(channelMask);
		plusBgCanvas_->Submit(channelMask);
		plusArrowCanvas_->Submit(channelMask);
		ringCanvas_->Submit(channelMask);
	}
}
