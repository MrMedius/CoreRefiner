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

namespace Text
{
    TextRenderer::TextRenderer(TextCodex& codex) : codex_(codex) {}

    void TextRenderer::EnsureFormatAndLayout_(const RenderRequest& req, float layoutW, float layoutH)
    {
        codex_.Init();

        const bool wChanged = (std::abs(lastLayoutW_ - layoutW) > 0.01f);

        // 文本变化：UTF-8 -> UTF-16
        const std::wstring w = ToWideUtf8(req.utf8Text);
        const bool textChanged = (w != textW_);
        if (textChanged) textW_ = w;

        if (!layout_ || textChanged || wChanged)
        {
            // 注意：TextFormat 的 family 目前仍以系统族名为主（你已验证可用）
            // 文件主字体的“作为 layout 主族名”需要自定义 font collection（后续增强点）。
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

            format_->SetTextAlignment(req.style.textAlign);
            format_->SetParagraphAlignment(req.style.paragraphAlign);
            format_->SetWordWrapping(req.style.wrapping);

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

            if (req.style.lineSpacing > 0.0f)
                layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, req.style.lineSpacing, req.style.lineSpacing * 0.8f);

            layout_ = std::move(layout);
            lastLayoutW_ = layoutW;

            ApplySpans_(req);
        }
    }

    void TextRenderer::ApplySpans_(const RenderRequest& req)
    {
        effects_.clear();
        if (!layout_) return;

        for (const auto& sp : req.spans)
        {
            if (sp.length == 0) continue;
            DWRITE_TEXT_RANGE range{ sp.start, sp.length };

            if (sp.weight) layout_->SetFontWeight(*sp.weight, range);
            if (sp.style)  layout_->SetFontStyle(*sp.style, range);
            if (sp.stretch) layout_->SetFontStretch(*sp.stretch, range);
            if (sp.fontFamily) layout_->SetFontFamilyName(sp.fontFamily->c_str(), range);

            if (sp.color)
            {
                Microsoft::WRL::ComPtr<IUnknown> eff;
                eff.Attach(static_cast<IUnknown*>(new ColorEffect(*sp.color)));
                layout_->SetDrawingEffect(eff.Get(), range);
                effects_.push_back(std::move(eff));
            }
        }
    }

    MeasureResult TextRenderer::Measure(const RenderRequest& req)
    {
        const float layoutW = std::max(1.0f, req.maxWidthPx);
        const float layoutH = 100000.0f;
        EnsureFormatAndLayout_(req, layoutW, layoutH);

        DWRITE_TEXT_METRICS m{};
        layout_->GetMetrics(&m);

        DWRITE_OVERHANG_METRICS o{};
        layout_->GetOverhangMetrics(&o);

        const float overhangX = std::max(0.0f, o.left) + std::max(0.0f, o.right);
        const float overhangY = std::max(0.0f, o.top) + std::max(0.0f, o.bottom);

        const float finalW = layoutW + overhangX + float(req.paddingPx * 2);
        const float finalH = std::max(1.0f, m.height) + overhangY + float(req.paddingPx * 2);

        MeasureResult r;
        r.widthPx = std::max<UINT>(1u, static_cast<UINT>(std::ceil(finalW)));
        r.heightPx = std::max<UINT>(1u, static_cast<UINT>(std::ceil(finalH)));
        return r;
    }

    void TextRenderer::Render(const RenderRequest& req, Canvas& canvas)
    {
        // Fixed：宽度来自 canvas；Auto：宽度来自 req.maxWidthPx
        float layoutW = 1.0f;
        float layoutH = 1.0f;

        if (req.canvasMode == CanvasMode::Fixed)
        {
            layoutW = std::max(1.0f, float(canvas.GetCanvasWidth() - req.paddingPx * 2));
            layoutH = std::max(1.0f, float(canvas.GetCanvasHeight() - req.paddingPx * 2));
        }
        else
        {
            layoutW = std::max(1.0f, req.maxWidthPx);
            layoutH = 100000.0f;
        }

        EnsureFormatAndLayout_(req, layoutW, layoutH);

        if (req.canvasMode == CanvasMode::Auto)
        {
            const auto ms = Measure(req);
            if (canvas.GetCanvasWidth() != ms.widthPx || canvas.GetCanvasHeight() != ms.heightPx)
                canvas.Resize(ms.widthPx, ms.heightPx);
        }

        if (req.clearMode == ClearMode::Clear)
            canvas.Clear(req.backgroundColor);

        DWRITE_OVERHANG_METRICS o{};
        layout_->GetOverhangMetrics(&o);

        const float originX = float(req.paddingPx) + std::max(0.0f, o.left);
        const float originY = float(req.paddingPx) + std::max(0.0f, o.top);

        // 堆对象 + ComPtr 持有，避免栈对象被 Release() delete
        Microsoft::WRL::ComPtr<IDWriteTextRenderer> renderer;
        renderer.Attach(static_cast<IDWriteTextRenderer*>(new DWriteLayoutRenderer(codex_, canvas, req.defaultColor)));

        layout_->Draw(nullptr, renderer.Get(), originX, originY);

        canvas.NotifyPixelsChanged();
    }
}