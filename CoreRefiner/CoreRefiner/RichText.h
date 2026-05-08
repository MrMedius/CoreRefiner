#pragma once
#include "Win.h"
#include "Colors.h"
#include <Unknwn.h>

namespace Text
{
    struct __declspec(uuid("6D5A0A8B-2F6C-4D52-9CC0-2E0C1C70A0F2")) IColorEffect : public IUnknown
    {
        virtual Color GetColor() const noexcept = 0;
    };

    class ColorEffect final : public IColorEffect
    {
    public:
        explicit ColorEffect(Color c) : color_(c) {}

        // IColorEffect
        Color GetColor() const noexcept override { return color_; }

        // IUnknown
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
        ULONG __stdcall AddRef() override;
        ULONG __stdcall Release() override;

    private:
        ~ColorEffect() = default;

    private:
        ULONG ref_ = 1;
        Color color_;
    };
}