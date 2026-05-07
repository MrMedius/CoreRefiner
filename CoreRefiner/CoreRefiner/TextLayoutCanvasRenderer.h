#pragma once
#include <dwrite.h>
#include "WRL.h"
#include "Canvas.h"

class TextCodex;
class Canvas;
class Color;

/**
 * 把 IDWriteTextLayout::Draw 的 glyph-run 回调转换为写入 Canvas 的像素。
 * 阶段5：只做“单色文本”。
 * 阶段6：再用 clientDrawingEffect 做富文本颜色/粗体等。
 */
class TextLayoutCanvasRenderer final : public IDWriteTextRenderer
{
public:
    TextLayoutCanvasRenderer(TextCodex& codex, Canvas& canvas, Color color);

    // IUnknown
    IFACEMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override;
    IFACEMETHOD_(ULONG, AddRef)() override;
    IFACEMETHOD_(ULONG, Release)() override;

    // IDWritePixelSnapping
    IFACEMETHOD(IsPixelSnappingDisabled)(void* clientDrawingContext, BOOL* isDisabled) override;
    IFACEMETHOD(GetCurrentTransform)(void* clientDrawingContext, DWRITE_MATRIX* transform) override;
    IFACEMETHOD(GetPixelsPerDip)(void* clientDrawingContext, FLOAT* pixelsPerDip) override;

    // IDWriteTextRenderer
    IFACEMETHOD(DrawGlyphRun)(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        const DWRITE_GLYPH_RUN* glyphRun,
        const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription,
        IUnknown* clientDrawingEffect) override;

    IFACEMETHOD(DrawUnderline)(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        const DWRITE_UNDERLINE* underline,
        IUnknown* clientDrawingEffect) override;

    IFACEMETHOD(DrawStrikethrough)(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        const DWRITE_STRIKETHROUGH* strikethrough,
        IUnknown* clientDrawingEffect) override;

    IFACEMETHOD(DrawInlineObject)(
        void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect) override;

private:
    ULONG ref_ = 1;
    TextCodex& codex_;
    Canvas& canvas_;
    Color color_;
};