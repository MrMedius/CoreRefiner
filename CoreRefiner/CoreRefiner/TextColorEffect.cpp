#include "TextColorEffect.h"

HRESULT __stdcall TextColorEffect::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) return E_POINTER;
    *ppvObject = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ITextColorEffect))
    {
        *ppvObject = static_cast<ITextColorEffect*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG __stdcall TextColorEffect::AddRef()
{
    return ++ref_;
}

ULONG __stdcall TextColorEffect::Release()
{
    const ULONG r = --ref_;
    if (r == 0)
        delete this;
    return r;
}