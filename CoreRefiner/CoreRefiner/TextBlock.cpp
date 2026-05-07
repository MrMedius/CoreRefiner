#include "TextBlock.h"

#include "TextCodex.h"
#include "Util.h"
#include "Canvas.h"
#include "Colors.h"
#include "TextLayoutCanvasRenderer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

void TextBlock::SetTextUtf8(std::string utf8)
{
    textUtf8_ = std::move(utf8);
    textW_ = ToWideUtf8(textUtf8_);
    dirty_ = true;
}

void TextBlock::SetMaxWidth(float maxWidthPx)
{
    maxWidthPx_ = std::max(1.0f, maxWidthPx);
    dirty_ = true;
}

void TextBlock::SetPadding(int paddingPx)
{
    paddingPx_ = std::max(0, paddingPx);
    dirty_ = true;
}

void TextBlock::SetStyle(const Style& style)
{
    style_ = style;
    dirty_ = true;
}

void TextBlock::EnsureLayout_(float layoutWidthPx, float layoutHeightPx)
{
    auto& tc = TextCodex::Get();
    tc.Init();

    const bool widthChanged = (std::abs(lastLayoutWidth_ - layoutWidthPx) > 0.01f);

    if (!dirty_ && layout_ && !widthChanged)
        return;

    format_ = tc.GetSystemFormat(style_.fontFamily, style_.fontSize, style_.weight, style_.fontStyle, style_.stretch);

    format_->SetTextAlignment(style_.textAlign);
    format_->SetParagraphAlignment(style_.paragraphAlign);
    format_->SetWordWrapping(style_.wrapping);

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = tc.GetFactory()->CreateTextLayout(
        textW_.c_str(),
        static_cast<UINT32>(textW_.size()),
        format_.Get(),
        layoutWidthPx,
        layoutHeightPx,
        &layout
    );
    if (FAILED(hr) || !layout)
        throw std::runtime_error("TextBlock: CreateTextLayout failed");

    if (style_.lineSpacing > 0.0f)
    {
        // baseline 这里用一个经验比例；后续要更严谨可用 font metrics 算
        layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, style_.lineSpacing, style_.lineSpacing * 0.8f);
    }

    layout_ = std::move(layout);
    dirty_ = false;
    lastLayoutWidth_ = layoutWidthPx;
}

void TextBlock::Measure(UINT& outW, UINT& outH)
{
    // Measure 以“模式B”为基准：宽度由 maxWidthPx_ 决定
    const float layoutW = std::max(1.0f, maxWidthPx_);
    const float layoutH = 100000.0f; // 让 DWrite 自己算高度
    EnsureLayout_(layoutW, layoutH);

    DWRITE_TEXT_METRICS m{};
    layout_->GetMetrics(&m);

    DWRITE_OVERHANG_METRICS o{};
    layout_->GetOverhangMetrics(&o);

    const float overhangX = std::max(0.0f, o.left) + std::max(0.0f, o.right);
    const float overhangY = std::max(0.0f, o.top) + std::max(0.0f, o.bottom);

    const float finalW = layoutW + overhangX + float(paddingPx_ * 2);
    const float finalH = std::max(1.0f, m.height) + overhangY + float(paddingPx_ * 2);

    outW = std::max<UINT>(1u, static_cast<UINT>(std::ceil(finalW)));
    outH = std::max<UINT>(1u, static_cast<UINT>(std::ceil(finalH)));
}

void TextBlock::RenderToCanvasFixed(Canvas& canvas, Color textColor)
{
    // 模式A：不允许改变 canvas 像素尺寸
    const float layoutW = std::max(1.0f, float(canvas.GetCanvasWidth() - paddingPx_ * 2));
    const float layoutH = std::max(1.0f, float(canvas.GetCanvasHeight() - paddingPx_ * 2));

    EnsureLayout_(layoutW, layoutH);

    canvas.Clear(Colors::None);

    // origin：加 padding，并抵消 overhang，避免裁切
    DWRITE_OVERHANG_METRICS o{};
    layout_->GetOverhangMetrics(&o);

    const float originX = float(paddingPx_) + std::max(0.0f, o.left);
    const float originY = float(paddingPx_) + std::max(0.0f, o.top);

    TextLayoutCanvasRenderer renderer(TextCodex::Get(), canvas, textColor);
    layout_->Draw(nullptr, &renderer, originX, originY);

    canvas.NotifyPixelsChanged();
}

void TextBlock::RenderToCanvasAuto(Canvas& canvas, Color textColor)
{
    // 模式B：Measure -> Resize
    UINT w = 1, h = 1;
    Measure(w, h);

    if (canvas.GetCanvasWidth() != w || canvas.GetCanvasHeight() != h)
        canvas.Resize(w, h);

    canvas.Clear(Colors::None);

    // 对齐 origin（抵消 overhang）
    DWRITE_OVERHANG_METRICS o{};
    layout_->GetOverhangMetrics(&o);

    const float originX = float(paddingPx_) + std::max(0.0f, o.left);
    const float originY = float(paddingPx_) + std::max(0.0f, o.top);

    TextLayoutCanvasRenderer renderer(TextCodex::Get(), canvas, textColor);
    layout_->Draw(nullptr, &renderer, originX, originY);

    canvas.NotifyPixelsChanged();
}