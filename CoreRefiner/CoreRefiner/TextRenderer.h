#pragma once
#include "TextTypes.h"
#include "WRL.h"

class TextCodex;
class Canvas;

namespace Text
{
    class TextRenderer
    {
    public:
        explicit TextRenderer(TextCodex& codex);

        MeasureResult Measure(const RenderRequest& req);
        void Render(const RenderRequest& req, Canvas& canvas);

    private:
        void EnsureFormatAndLayout_(const RenderRequest& req, float layoutW, float layoutH);
        void ApplySpans_(const RenderRequest& req);

    private:
        TextCodex& codex_;

        // cache：以最近一次 request 为主（第一期简单缓存，后续可做hash缓存）
        std::wstring textW_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> format_;
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout_;
        float lastLayoutW_ = -1.0f;

        // 保持 effect 生命周期
        std::vector<Microsoft::WRL::ComPtr<IUnknown>> effects_;
    };
}