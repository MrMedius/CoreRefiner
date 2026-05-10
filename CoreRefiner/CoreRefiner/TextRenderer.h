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
        static float GetContentLayoutWidth_(const RenderRequest& req, const Canvas* canvasNullable);
        static float GetContentLayoutHeight_(const RenderRequest& req, const Canvas* canvasNullable);

        void EnsureFormatAndLayout_(const RenderRequest& req, float layoutW, float layoutH);
        void ApplySpans_(const RenderRequest& req);

        // Called when returning the TextCodex pool to prevent layout/format cache from leaking into the next BeginDraw.
        void ResetForPool_() noexcept;

    private:
        TextCodex& codex_;

		// Cache the UTF-16 text for layout; only updated when the UTF-8 text changes to avoid repeated conversions.
        std::wstring textW_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> format_;
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout_;
        float lastLayoutW_ = -1.0f;
        float lastLayoutH_ = -1.0f;

		// Cache the last RenderRequest parameters that affect layout/format to determine when to update them (layout is more expensive to create than format, so it's checked separately).
        Style lastStyle_{};
        FontSource lastPrimaryFont_{};
        std::vector<Span> lastSpans_{};
        bool hasLayoutSnapshot_ = false;

        std::vector<Microsoft::WRL::ComPtr<IUnknown>> effects_;
    };
}