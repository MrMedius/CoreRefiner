#include "TextRenderer.h"

#include "TextCodex.h"
#include "RichText.h"
#include "DWriteLayoutRenderer.h"
#include "Util.h"
#include "Canvas.h"
#include "Colors.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// Helper functions for comparing RenderRequest parameters to determine when to update the cached TextFormat and TextLayout (to avoid unnecessary updates and expensive layout creation).
namespace
{
    constexpr float kUnboundedLayoutWidth = 1.0e6f;
    constexpr float kAutoLayoutHeight = 100000.0f;

    bool FontSourceEqual(const Text::FontSource& a, const Text::FontSource& b)
    {
        if (a.kind != b.kind)
            return false;
        if (a.kind == Text::FontSourceKind::SystemFamily)
            return a.systemFamily == b.systemFamily;
        return a.fontFilePath == b.fontFilePath;
    }
    template<typename T>
    bool OptionalEqual(const std::optional<T>& a, const std::optional<T>& b)
    {
        if (a.has_value() != b.has_value())
            return false;
        if (!a.has_value())
            return true;
        return *a == *b;
    }
    bool SpanEqual(const Text::Span& a, const Text::Span& b)
    {
        return a.start == b.start
            && a.length == b.length
            && OptionalEqual(a.weight, b.weight)
            && OptionalEqual(a.style, b.style)
            && OptionalEqual(a.stretch, b.stretch)
            && OptionalEqual(a.fontFamily, b.fontFamily)
            && OptionalEqual(a.underline, b.underline)
            && OptionalEqual(a.strikethrough, b.strikethrough)
            && OptionalEqual(a.color, b.color);
    }
    bool SpansEqual(const std::vector<Text::Span>& a, const std::vector<Text::Span>& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!SpanEqual(a[i], b[i]))
                return false;
        }
        return true;
    }
    bool StyleEqual(const Text::Style& a, const Text::Style& b)
    {
        return a.fontSize == b.fontSize
            && a.weight == b.weight
            && a.fontStyle == b.fontStyle
            && a.stretch == b.stretch
            && a.textAlign == b.textAlign
            && a.paragraphAlign == b.paragraphAlign
            && a.wrapping == b.wrapping
            && a.wordWrapEnabled == b.wordWrapEnabled
            && a.lineSpacing == b.lineSpacing;
    }
    bool SnapshotMatches(
        const Text::RenderRequest& req,
        const Text::Style& st,
        const Text::FontSource& pf,
        const std::vector<Text::Span>& sp,
        bool hasSnap)
    {
        if (!hasSnap)
            return false;
        return StyleEqual(req.style, st)
            && FontSourceEqual(req.primaryFont, pf)
            && SpansEqual(req.spans, sp);
    }

	[[nodiscard]] float ResolveBoxWidth_(const Text::RenderRequest& req, const Canvas* canvasNullable)
	{
		if (req.HasDestRect())
		{
			return req.destWPx;
		}
		if (canvasNullable != nullptr)
		{
			return static_cast<float>(canvasNullable->GetCanvasWidth());
		}
		return req.maxWidthPx;
	}

	[[nodiscard]] float ResolveBoxHeight_(const Text::RenderRequest& req, const Canvas* canvasNullable)
	{
		if (req.HasDestRect())
		{
			return req.destHPx;
		}
		if (canvasNullable != nullptr)
		{
			return static_cast<float>(canvasNullable->GetCanvasHeight());
		}
		return kAutoLayoutHeight;
	}
}


namespace Text
{
    TextRenderer::TextRenderer(TextCodex& codex) : codex_(codex) {}

    MeasureResult TextRenderer::Measure(const RenderRequest& req)
    {
        const float layoutW = GetContentLayoutWidth_(req, nullptr);
        const float layoutH = GetContentLayoutHeight_(req, nullptr);
        EnsureFormatAndLayout_(req, layoutW, layoutH);

        DWRITE_TEXT_METRICS m{};
        layout_->GetMetrics(&m);

        DWRITE_OVERHANG_METRICS o{};
        layout_->GetOverhangMetrics(&o);

        const float pad = float(req.paddingPx);
        const float flowW = std::max(1.0f, m.width);
        const float flowH = std::max(1.0f, m.height);

        const float baseW = pad * 2.0f + flowW + std::max(0.0f, o.left) + std::max(0.0f, o.right);
        const float baseH = pad * 2.0f + flowH + std::max(0.0f, o.top) + std::max(0.0f, o.bottom);
        
        const float finalW = baseW + std::fabs(req.drawOffsetXPx);
        const float finalH = baseH + std::fabs(req.drawOffsetYPx);

        MeasureResult r;
        r.widthPx = std::max<UINT>(1u, static_cast<UINT>(std::ceil(finalW)));
        r.heightPx = std::max<UINT>(1u, static_cast<UINT>(std::ceil(finalH)));
        return r;
    }

    void TextRenderer::Render(const RenderRequest& req, Canvas& canvas)
    {
		// Step 1 : CanvasMode + Determine layout width and height based on the RenderRequest and canvas (for Fixed mode, it's based on canvas size minus padding; for Auto mode, it's based on maxWidthPx and a large value for height to allow auto-sizing).
        const float layoutW = GetContentLayoutWidth_(req, &canvas);
        const float layoutH = GetContentLayoutHeight_(req, &canvas);

        // Step 2 : EnsureFormatAndLayout
        EnsureFormatAndLayout_(req, layoutW, layoutH);

        // Step 3 : Auto measures the text and resizes the canvas if needed
        if (req.canvasMode == CanvasMode::Auto)
        {
            const auto ms = Measure(req);
            if (canvas.GetCanvasWidth() != ms.widthPx || canvas.GetCanvasHeight() != ms.heightPx)
                canvas.Resize(ms.widthPx, ms.heightPx);
        }

        // Step 4 : ClearMode : Fixed->works properly, Auto->clear to none or specified color
        if (req.clearMode == ClearMode::Clear)
            canvas.Clear(req.backgroundColor);

        // Step 5 : Draw text to canvas
        DWRITE_OVERHANG_METRICS o{};
        layout_->GetOverhangMetrics(&o);

		const float destX = req.HasDestRect() ? req.destXPx : 0.0f;
		const float destY = req.HasDestRect() ? req.destYPx : 0.0f;
        float originX = destX + float(req.paddingPx) + std::max(0.0f, o.left) + req.drawOffsetXPx;
        float originY = destY + float(req.paddingPx) + std::max(0.0f, o.top) + req.drawOffsetYPx;

        Microsoft::WRL::ComPtr<IDWriteTextRenderer> renderer;
        bool clipEnabled = false;
        int clipX0 = 0;
        int clipY0 = 0;
        int clipX1 = 0;
        int clipY1 = 0;
        if (req.HasDestRect())
        {
            clipEnabled = true;
            clipX0 = static_cast<int>(std::floor(req.destXPx));
            clipY0 = static_cast<int>(std::floor(req.destYPx));
            clipX1 = static_cast<int>(std::ceil(req.destXPx + req.destWPx));
            clipY1 = static_cast<int>(std::ceil(req.destYPx + req.destHPx));
        }
        renderer.Attach(static_cast<IDWriteTextRenderer*>(new DWriteLayoutRenderer(
            codex_,
            canvas,
            req.defaultColor,
            clipEnabled,
            clipX0,
            clipY0,
            clipX1,
            clipY1)));

        layout_->Draw(nullptr, renderer.Get(), originX, originY);

        canvas.NotifyPixelsChanged();
    }

    float TextRenderer::GetContentLayoutWidth_(const RenderRequest& req, const Canvas* canvasNullable)
    {
		if (req.canvasMode == CanvasMode::Fixed)
		{
			const int inner = static_cast<int>(ResolveBoxWidth_(req, canvasNullable)) - req.paddingPx * 2;
			return std::max(1.0f, float(std::max(0, inner)));
		}
        if (!req.style.wordWrapEnabled)
            return kUnboundedLayoutWidth;
        return std::max(1.0f, req.maxWidthPx);
    }
    float TextRenderer::GetContentLayoutHeight_(const RenderRequest& req, const Canvas* canvasNullable)
    {
        if (req.canvasMode == CanvasMode::Fixed)
        {
			const int inner = static_cast<int>(ResolveBoxHeight_(req, canvasNullable)) - req.paddingPx * 2;
            return std::max(1.0f, float(std::max(0, inner)));
        }
        return kAutoLayoutHeight;
    }


    void TextRenderer::EnsureFormatAndLayout_(const RenderRequest& req, float layoutW, float layoutH)
    {
        codex_.Init();

		// Determine if we can reuse the cached TextFormat and TextLayout based on the current RenderRequest parameters and layout width (layout height is not checked because it doesn't affect layout creation and is usually set to a large value for auto height).
        const bool wChanged = (std::abs(lastLayoutW_ - layoutW) > 0.01f);
        const bool hChanged = (std::abs(lastLayoutH_ - layoutH) > 0.01f);

        const std::wstring w = ToWideUtf8(req.text);
        const bool textChanged = (w != textW_);
        if (textChanged) textW_ = w;
        
        const bool metaOk = SnapshotMatches(req, lastStyle_, lastPrimaryFont_, lastSpans_, hasLayoutSnapshot_);
        const bool needRebuild = !layout_ || textChanged || wChanged || hChanged || !metaOk;

        if (!needRebuild)
            return;
        
		// Rebuild TextFormat and TextLayout
        if (req.primaryFont.kind == FontSourceKind::SystemFamily)
        {
            format_ = codex_.GetSystemFormat(
                req.primaryFont.systemFamily,
                req.style.fontSize,
                req.style.weight,
                req.style.fontStyle,
                req.style.stretch
            );
        }
        else
        {
            format_ = codex_.GetFileTextFormat(
                req.primaryFont.fontFilePath,
                req.style.fontSize,
                req.style.weight,
                req.style.fontStyle,
                req.style.stretch
            );
        }

		// Set common TextFormat properties that affect layout but not cached in the TextFormat (alignment and wrapping).
        format_->SetTextAlignment(req.style.textAlign);
        format_->SetParagraphAlignment(req.style.paragraphAlign);

		// If word wrapping is disabled, set it to NO_WRAP; otherwise use the specified wrapping mode (default is WRAP).
        const DWRITE_WORD_WRAPPING wrapMode = req.style.wordWrapEnabled
            ? req.style.wrapping
            : DWRITE_WORD_WRAPPING_NO_WRAP;
        format_->SetWordWrapping(wrapMode);

		// Create TextLayout with the specified layout width and height (height can be large for auto height); if line spacing is specified, set it to uniform with the given value.
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        HRESULT hr = codex_.GetFactory()->CreateTextLayout(
            textW_.c_str(),
            static_cast<UINT32>(textW_.size()),
            format_.Get(),
            layoutW,
            layoutH,
            &layout
        );
        if (FAILED(hr) || !layout)
            throw std::runtime_error("TextRenderer: CreateTextLayout failed");

		// Set uniform line spacing if specified (lineSpacing > 0); the baseline is set to 80% of the line spacing value to achieve a more balanced appearance (this is a common practice but can be adjusted as needed).
        if (req.style.lineSpacing > 0.0f)
        {
            const float defaultLineHeight = req.style.fontSize * 1.2f;
            const float uniformHeight = defaultLineHeight + req.style.lineSpacing;
            const float baseline = std::clamp(
                req.style.fontSize * 0.86f,
                uniformHeight * 0.45f,
                std::max(uniformHeight - 1.0f, req.style.fontSize * 0.5f));
            layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, uniformHeight, baseline);
        }

		// Cache the created layout and the parameters that affect it for future reuse.
        layout_ = std::move(layout);
        lastLayoutW_ = layoutW;
        lastLayoutH_ = layoutH;

        lastStyle_ = req.style;
        lastPrimaryFont_ = req.primaryFont;
        lastSpans_ = req.spans;
        hasLayoutSnapshot_ = true;

        ApplySpans_(req);
    }

    void TextRenderer::ApplySpans_(const RenderRequest& req)
    {
        effects_.clear();
        if (!layout_) return;

        for (const auto& sp : req.spans)
        {
            if (sp.length == 0) continue;
            DWRITE_TEXT_RANGE range{ sp.start, sp.length };

            if (sp.weight)      layout_->SetFontWeight(*sp.weight, range);
            if (sp.style)       layout_->SetFontStyle(*sp.style, range);
            if (sp.stretch)     layout_->SetFontStretch(*sp.stretch, range);
            if (sp.fontFamily)  layout_->SetFontFamilyName(sp.fontFamily->c_str(), range);
			
            if (sp.underline.has_value())       layout_->SetUnderline(*sp.underline, range);
			if (sp.strikethrough.has_value())   layout_->SetStrikethrough(*sp.strikethrough, range);
            
            if (sp.color.has_value())
            {
                Microsoft::WRL::ComPtr<IUnknown> eff;
                eff.Attach(static_cast<IUnknown*>(new ColorEffect(*sp.color)));
                layout_->SetDrawingEffect(eff.Get(), range);
                effects_.push_back(std::move(eff));
            }
        }
    }

  void TextRenderer::ResetForPool_() noexcept
    {
        textW_.clear();
        format_.Reset();
        layout_.Reset();
        lastLayoutW_ = -1.0f;
        lastLayoutH_ = -1.0f;
        lastStyle_ = {};
        lastPrimaryFont_ = {};
        lastSpans_.clear();
        hasLayoutSnapshot_ = false;
        effects_.clear();
    }
}