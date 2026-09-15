#pragma once
#include <dwrite.h>
#include "WRL.h"
#include "Canvas.h"
class TextCodex;

namespace Text
{
    class DWriteLayoutRenderer final : public IDWriteTextRenderer
    {
    public:
        // clipEnabled：只画 [clipX0, clipX1) × [clipY0, clipY1)，给 Fixed dest 裁超框字。
        DWriteLayoutRenderer(
            TextCodex& codex,
            Canvas& canvas,
            Color defaultColor,
            bool clipEnabled = false,
            int clipX0 = 0,
            int clipY0 = 0,
            int clipX1 = 0,
            int clipY1 = 0);

        // IUnknown
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
        ULONG __stdcall AddRef() override;
        ULONG __stdcall Release() override;

        // IDWritePixelSnapping
        HRESULT __stdcall IsPixelSnappingDisabled(void*, BOOL* isDisabled) override;
        HRESULT __stdcall GetCurrentTransform(void*, DWRITE_MATRIX* transform) override;
        HRESULT __stdcall GetPixelsPerDip(void*, FLOAT* pixelsPerDip) override;

        // IDWriteTextRenderer
        HRESULT __stdcall DrawGlyphRun(
            void*,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_MEASURING_MODE,
            const DWRITE_GLYPH_RUN* glyphRun,
            const DWRITE_GLYPH_RUN_DESCRIPTION*,
            IUnknown* clientDrawingEffect) override;

        HRESULT __stdcall DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) override;
        HRESULT __stdcall DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) override;
        HRESULT __stdcall DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*) override;

    private:
        ~DWriteLayoutRenderer() = default;

    private:
        ULONG ref_ = 1;
        TextCodex& codex_;
        Canvas& canvas_;
        Color defaultColor_;
        bool clipEnabled_{ false };
        int clipX0_{ 0 };
        int clipY0_{ 0 };
        int clipX1_{ 0 };
        int clipY1_{ 0 };
    };
}