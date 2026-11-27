#include "DWriteLayoutRenderer.h"

#include "TextCodex.h"
#include "RichText.h"
#include "Canvas.h"
#include "Surface.h"

#include <algorithm>
#include <cmath>

namespace
{
    Color ResolveDrawColor(Color fallback, IUnknown* clientDrawingEffect)
    {
        if (!clientDrawingEffect)
            return fallback;
        Text::IColorEffect* eff = nullptr;
        if (SUCCEEDED(clientDrawingEffect->QueryInterface(__uuidof(Text::IColorEffect), reinterpret_cast<void**>(&eff))) && eff)
        {
            const Color c = eff->GetColor();
            eff->Release();
            return c;
        }
        return fallback;
    }

    // 轴对齐矩形填充；可再与 dest clip 求交。
    void FillSolidRect(
        Canvas& canvas,
        int x0,
        int y0,
        int x1,
        int y1,
        Color color,
        bool clipEnabled,
        int clipX0,
        int clipY0,
        int clipX1,
        int clipY1)
    {
        Surface& surface = canvas.GetSurface();
        const int w = static_cast<int>(surface.GetWidth());
        const int h = static_cast<int>(surface.GetHeight());
        x0 = std::max(0, x0);
        y0 = std::max(0, y0);
        x1 = std::min(w, x1);
        y1 = std::min(h, y1);
        if (clipEnabled)
        {
            x0 = std::max(x0, clipX0);
            y0 = std::max(y0, clipY0);
            x1 = std::min(x1, clipX1);
            y1 = std::min(y1, clipY1);
        }
        if (x0 >= x1 || y0 >= y1)
            return;

        for (int y = y0; y < y1; ++y)
        {
            for (int x = x0; x < x1; ++x)
                surface.PutPixel(x, y, color);
        }
    }

    void FillUnderlineBand(
        Canvas& canvas,
        float baselineOriginX,
        float baselineOriginY,
        const DWRITE_UNDERLINE* u,
        Color color,
        bool clipEnabled,
        int clipX0,
        int clipY0,
        int clipX1,
        int clipY1)
    {
        if (!u || u->width <= 0.0f || u->thickness <= 0.0f)
            return;

        const float left = baselineOriginX;
        const float top = baselineOriginY + u->offset;
        const float width = u->width;
        const float thick = std::max(u->thickness, 1.0f);

        const int x0 = static_cast<int>(std::floor(left));
        const int y0 = static_cast<int>(std::floor(top));
        const int x1 = static_cast<int>(std::ceil(left + width));
        const int y1 = static_cast<int>(std::ceil(top + thick));

        FillSolidRect(canvas, x0, y0, x1, y1, color, clipEnabled, clipX0, clipY0, clipX1, clipY1);
    }

    void FillStrikethroughBand(
        Canvas& canvas,
        float baselineOriginX,
        float baselineOriginY,
        const DWRITE_STRIKETHROUGH* s,
        Color color,
        bool clipEnabled,
        int clipX0,
        int clipY0,
        int clipX1,
        int clipY1)
    {
        if (!s || s->width <= 0.0f || s->thickness <= 0.0f)
            return;

        const float left = baselineOriginX;
        const float top = baselineOriginY + s->offset;
        const float width = s->width;
        const float thick = std::max(s->thickness, 1.0f);

        const int x0 = static_cast<int>(std::floor(left));
        const int y0 = static_cast<int>(std::floor(top));
        const int x1 = static_cast<int>(std::ceil(left + width));
        const int y1 = static_cast<int>(std::ceil(top + thick));

        FillSolidRect(canvas, x0, y0, x1, y1, color, clipEnabled, clipX0, clipY0, clipX1, clipY1);
    }
}

namespace Text
{
    DWriteLayoutRenderer::DWriteLayoutRenderer(
        TextCodex& codex,
        Canvas& canvas,
        Color defaultColor,
        bool clipEnabled,
        int clipX0,
        int clipY0,
        int clipX1,
        int clipY1)
        :
        codex_(codex),
        canvas_(canvas),
        defaultColor_(defaultColor),
        clipEnabled_(clipEnabled),
        clipX0_(clipX0),
        clipY0_(clipY0),
        clipX1_(clipX1),
        clipY1_(clipY1)
    {
    }

    HRESULT __stdcall DWriteLayoutRenderer::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;

        if (riid == __uuidof(IUnknown) ||
            riid == __uuidof(IDWriteTextRenderer) ||
            riid == __uuidof(IDWritePixelSnapping))
        {
            *ppvObject = static_cast<IDWriteTextRenderer*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG __stdcall DWriteLayoutRenderer::AddRef() { return ++ref_; }

    ULONG __stdcall DWriteLayoutRenderer::Release()
    {
        const ULONG r = --ref_;
        if (r == 0) delete this;
        return r;
    }

    HRESULT __stdcall DWriteLayoutRenderer::IsPixelSnappingDisabled(void*, BOOL* isDisabled)
    {
        if (!isDisabled) return E_POINTER;
        *isDisabled = FALSE;
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::GetCurrentTransform(void*, DWRITE_MATRIX* transform)
    {
        if (!transform) return E_POINTER;
        *transform = DWRITE_MATRIX{ 1,0,0,1,0,0 };
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::GetPixelsPerDip(void*, FLOAT* pixelsPerDip)
    {
        if (!pixelsPerDip) return E_POINTER;
        *pixelsPerDip = 1.0f;
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::DrawGlyphRun(
        void*,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE,
        const DWRITE_GLYPH_RUN* glyphRun,
        const DWRITE_GLYPH_RUN_DESCRIPTION*,
        IUnknown* clientDrawingEffect)
    {
        if (!glyphRun) return E_INVALIDARG;

        const Color c = ResolveDrawColor(defaultColor_, clientDrawingEffect);
        codex_.DrawGlyphRunToCanvas(
            canvas_,
            baselineOriginX,
            baselineOriginY,
            *glyphRun,
            c,
            clipEnabled_,
            clipX0_,
            clipY0_,
            clipX1_,
            clipY1_);
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::DrawUnderline(
        void*,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        const DWRITE_UNDERLINE* underline,
        IUnknown* clientDrawingEffect)
    {
        if (!underline) return E_INVALIDARG;
        const Color c = ResolveDrawColor(defaultColor_, clientDrawingEffect);
        FillUnderlineBand(
            canvas_,
            baselineOriginX,
            baselineOriginY,
            underline,
            c,
            clipEnabled_,
            clipX0_,
            clipY0_,
            clipX1_,
            clipY1_);
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::DrawStrikethrough(
        void*,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        const DWRITE_STRIKETHROUGH* strikethrough,
        IUnknown* clientDrawingEffect)
    {
        if (!strikethrough) return E_INVALIDARG;
        const Color c = ResolveDrawColor(defaultColor_, clientDrawingEffect);
        FillStrikethroughBand(
            canvas_,
            baselineOriginX,
            baselineOriginY,
            strikethrough,
            c,
            clipEnabled_,
            clipX0_,
            clipY0_,
            clipX1_,
            clipY1_);
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*)
    {
        return S_OK;
    }
}