#pragma once
#include <dwrite.h>
#include "WRL.h"

#include <string>
#include <string_view>

#include <vector>
#include <Unknwn.h>
#include "TextSpan.h"

class Canvas;
class Color;

/**
 * 阶段5：文本块
 * - 管理 TextFormat/TextLayout
 * - 自动换行（由 TextLayout 负责）
 * - Measure() 得到需要的像素尺寸
 * - 提供两种渲染模式：
 *   A) RenderToCanvasFixed: 写入既有画布（不 Resize，不变形）
 *   B) RenderToCanvasAuto : 自适应 Resize 画布（需要你显示时等比）
 */
class TextBlock
{
public:
    struct Style
    {
        std::wstring fontFamily = L"Segoe UI";
        float fontSize = 24.0f;
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;

        DWRITE_TEXT_ALIGNMENT textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WRAP;

        float lineSpacing = 0.0f; // 0 表示不强制；>0 则统一行距
    };

public:
    TextBlock() = default;

    void SetTextUtf8(std::string utf8);
    void SetMaxWidth(float maxWidthPx);   // 模式B常用：固定宽度，高度自适应
    void SetPadding(int paddingPx);
    void SetStyle(const Style& style);

    // 测量：输出建议像素尺寸
    void Measure(UINT& outW, UINT& outH);

    // 模式A：写入已有 canvas（canvas 像素尺寸固定，不 Resize）
    void RenderToCanvasFixed(Canvas& canvas, Color textColor);

    // 模式B：自适应 canvas（内部 Measure 并 Resize）
    void RenderToCanvasAuto(Canvas& canvas, Color textColor);

private:
    void EnsureLayout_(float layoutWidthPx, float layoutHeightPx);

private:
    bool dirty_ = true;

    std::string textUtf8_;
    std::wstring textW_;

    Style style_{};
    int paddingPx_ = 6;

    // 模式B：布局宽度（像素）
    float maxWidthPx_ = 320.0f;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format_;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout_;

    // 记录 layout 用过的宽度，避免每帧重建
    float lastLayoutWidth_ = -1.0f;


public:
    void SetSpans(std::vector<TextSpan> spans);
private:
    void ApplySpans_();
private:
    std::vector<TextSpan> spans_;
    std::vector<Microsoft::WRL::ComPtr<IUnknown>> effects_; // 持有 effect，避免被提前释放
};