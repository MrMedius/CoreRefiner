#include "RichText.h"

namespace Text
{
    HRESULT __stdcall ColorEffect::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IColorEffect))
        {
            *ppvObject = static_cast<IColorEffect*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG __stdcall ColorEffect::AddRef()
    {
        return ++ref_;
    }

    ULONG __stdcall ColorEffect::Release()
    {
        const ULONG r = --ref_;
        if (r == 0) delete this;
        return r;
    }
}