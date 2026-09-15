#pragma once

#include "Canvas.h"
#include "CanvasPixelDraw.h"
#include "Colors.h"
#include "TextCodex.h"

#include <string>
#include <vector>

namespace CanvasTextDraw
{
	// 说明书 / 货卡共用字体：YaHei，回退 Yu Gothic / Segoe。
	inline void FillRequest(Text::RenderRequest& rq, const std::string& text, float fontSize, Color color)
	{
		rq.text = text;
		rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		rq.fallbackFonts.clear();
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Yu Gothic UI"));
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
		rq.style.fontSize = fontSize;
		rq.defaultColor = color;
	}

	// wrap=false 按单行量宽；wrap=true 按 maxWidthPx 换行量高。
	[[nodiscard]] inline Text::MeasureResult Measure(
		const std::string& text,
		float fontSize,
		bool wrap,
		float maxWidthPx,
		const std::vector<Text::Span>& spans = {})
	{
		Text::MeasureResult result{};
		result.widthPx = 0u;
		result.heightPx = 0u;
		if (text.empty())
		{
			return result;
		}
		if (wrap && maxWidthPx <= 0.0f)
		{
			return result;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillRequest(rq, text, fontSize, Colors::White);
		rq.canvasMode = Text::CanvasMode::Auto;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.paddingPx = 0;
		rq.style.wordWrapEnabled = wrap;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		rq.maxWidthPx = wrap ? maxWidthPx : 4096.0f;
		rq.spans = spans;
		return ctx.Measure();
	}

	inline void Draw(
		Canvas& canvas,
		const std::string& text,
		float fontSize,
		int boxX,
		int boxY,
		int boxW,
		int boxH,
		Color color,
		bool wrap,
		const std::vector<Text::Span>& spans = {},
		int paddingPx = 0,
		DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
		DWRITE_PARAGRAPH_ALIGNMENT para = DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
		float drawOffsetXPx = 0.0f)
	{
		if (text.empty() || boxW <= 0 || boxH <= 0)
		{
			return;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillRequest(rq, text, fontSize, color);
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.style.wordWrapEnabled = wrap;
		rq.style.textAlign = align;
		rq.style.paragraphAlign = para;
		rq.paddingPx = paddingPx;
		rq.drawOffsetXPx = drawOffsetXPx;
		rq.SetDestRect(
			static_cast<float>(boxX),
			static_cast<float>(boxY),
			static_cast<float>(boxW),
			static_cast<float>(boxH));
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
	}

	inline void DrawHRule(Canvas& canvas, int x0, int x1, int y, Color c, int thickness = 1)
	{
		if (thickness < 1)
		{
			return;
		}
		for (int i = 0; i < thickness; ++i)
		{
			CanvasPixelDraw::DrawHLine(canvas, x0, x1, y + i, c);
		}
	}
}
