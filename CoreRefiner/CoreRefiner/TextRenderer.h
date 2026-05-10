#pragma once
#include "TextTypes.h"
#include "WRL.h"

#include <vector>

class TextCodex;
class Canvas;

namespace Text
{
    class TextRenderer
    {
        friend class TextCodex;

    public:
        explicit TextRenderer(TextCodex& codex);

        MeasureResult Measure(const RenderRequest& req);
        void Render(const RenderRequest& req, Canvas& canvas);

    private:
        void EnsureFormatAndLayout_(const RenderRequest& req, float layoutW, float layoutH);
        void ApplySpans_(const RenderRequest& req);

        // Called when returning the TextCodex pool to prevent layout/format cache from leaking into the next BeginDraw.
        void ResetForPool_() noexcept;

    private:
        TextCodex& codex_;

        std::wstring textW_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> format_;
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout_;
        float lastLayoutW_ = -1.0f;

        std::vector<Microsoft::WRL::ComPtr<IUnknown>> effects_;
    };
}