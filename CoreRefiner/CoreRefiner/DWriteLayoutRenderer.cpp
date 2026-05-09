#include "DWriteLayoutRenderer.h"

#include "TextCodex.h"
#include "RichText.h"
#include "Canvas.h"

namespace Text
{
    DWriteLayoutRenderer::DWriteLayoutRenderer(TextCodex& codex, Canvas& canvas, Color defaultColor)
        : codex_(codex), canvas_(canvas), defaultColor_(defaultColor)
    {}

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

        Color c = defaultColor_;

        if (clientDrawingEffect)
        {
            IColorEffect* eff = nullptr;
            if (SUCCEEDED(clientDrawingEffect->QueryInterface(__uuidof(IColorEffect), reinterpret_cast<void**>(&eff))) && eff)
            {
                c = eff->GetColor();
                eff->Release();
            }
        }

        codex_.DrawGlyphRunToCanvas(canvas_, baselineOriginX, baselineOriginY, *glyphRun, c);
        return S_OK;
    }

    HRESULT __stdcall DWriteLayoutRenderer::DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) { return S_OK; }
    HRESULT __stdcall DWriteLayoutRenderer::DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) { return S_OK; }
    HRESULT __stdcall DWriteLayoutRenderer::DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*) { return S_OK; }
}