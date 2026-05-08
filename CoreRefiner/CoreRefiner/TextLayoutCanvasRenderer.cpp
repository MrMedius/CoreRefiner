#include "TextLayoutCanvasRenderer.h"

#include "TextCodex.h"

#include "TextColorEffect.h"

TextLayoutCanvasRenderer::TextLayoutCanvasRenderer(TextCodex& codex, Canvas& canvas, Color color)
    : codex_(codex), canvas_(canvas), color_(color)
{
}

HRESULT TextLayoutCanvasRenderer::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG TextLayoutCanvasRenderer::AddRef()
{
    return ++ref_;
}

ULONG TextLayoutCanvasRenderer::Release()
{
    const ULONG r = --ref_;
    if (r == 0) delete this;
    return r;
}

HRESULT TextLayoutCanvasRenderer::IsPixelSnappingDisabled(void*, BOOL* isDisabled)
{
    if (!isDisabled) return E_POINTER;
    *isDisabled = FALSE;
    return S_OK;
}

HRESULT TextLayoutCanvasRenderer::GetCurrentTransform(void*, DWRITE_MATRIX* transform)
{
    if (!transform) return E_POINTER;
    *transform = DWRITE_MATRIX{ 1,0,0,1,0,0 };
    return S_OK;
}

HRESULT TextLayoutCanvasRenderer::GetPixelsPerDip(void*, FLOAT* pixelsPerDip)
{
    if (!pixelsPerDip) return E_POINTER;
    *pixelsPerDip = 1.0f;
    return S_OK;
}

HRESULT TextLayoutCanvasRenderer::DrawGlyphRun(
    void*,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE,
    const DWRITE_GLYPH_RUN* glyphRun,
    const DWRITE_GLYPH_RUN_DESCRIPTION*,
    IUnknown* clientDrawingEffect)
{
    if (!glyphRun) return E_INVALIDARG;
    Color drawColor = color_;
    if (clientDrawingEffect)
    {
        ITextColorEffect* pColorEffect = nullptr;
        if (SUCCEEDED(clientDrawingEffect->QueryInterface(__uuidof(ITextColorEffect), reinterpret_cast<void**>(&pColorEffect))) && pColorEffect)
        {
            drawColor = pColorEffect->GetColor();
            pColorEffect->Release();
        }
    }
    codex_.DrawGlyphRunToCanvas(canvas_, baselineOriginX, baselineOriginY, *glyphRun, drawColor);
    return S_OK;
}


HRESULT TextLayoutCanvasRenderer::DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*)
{
    return S_OK;
}

HRESULT TextLayoutCanvasRenderer::DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*)
{
    return S_OK;
}

HRESULT TextLayoutCanvasRenderer::DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*)
{
    return S_OK;
}