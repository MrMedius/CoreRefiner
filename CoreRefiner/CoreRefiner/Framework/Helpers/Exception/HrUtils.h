#pragma once
#include <Windows.h>
#include <string>
#include <sstream>

inline std::string HrToDescription(HRESULT hr)
{
    auto fmt = [&](DWORD langId) -> std::string {
        char* pMsg = nullptr;
        DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS;

        DWORD len = FormatMessageA(flags, nullptr, (DWORD)hr, langId, (LPSTR)&pMsg, 0, nullptr);
        if (len && pMsg)
        {
            std::string out(pMsg, pMsg + len);
            LocalFree(pMsg);
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
            return out;
        }
        return {};
    };

    // English
    if (auto s = fmt(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)); !s.empty())
        return s;

    // System default
    if (auto s = fmt(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)); !s.empty())
        return s;

    // Untranslated
    std::ostringstream oss;
    oss << "Untranslated HRESULT: 0x" << std::hex << std::uppercase << (unsigned long)hr;
    return oss.str();
}

inline const char* HrToName(HRESULT hr) noexcept
{
    switch (hr)
    {
    case S_OK: return "S_OK";
    case E_FAIL: return "E_FAIL";
    case E_INVALIDARG: return "E_INVALIDARG";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY";

    case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
    case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
    default: return "HRESULT (unmapped)";
    }
}