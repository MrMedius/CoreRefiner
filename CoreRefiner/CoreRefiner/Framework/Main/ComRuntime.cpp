#include "ComRuntime.h"
#include <wrl/client.h>

ComRuntime::ComRuntime(Model model)
    : model_(model)
{
    const DWORD coinit = (model_ == Model::STA)
        ? (COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)
        : (COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    const HRESULT hr = CoInitializeEx(nullptr, coinit);

    initialized_ = SUCCEEDED(hr);
}

ComRuntime::~ComRuntime()
{
    if (initialized_)
    {
        CoUninitialize();
    }
}