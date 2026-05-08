#pragma once
#include "Win.h"
#include "Colors.h"

#include <Unknwn.h>

struct __declspec(uuid("6D5A0A8B-2F6C-4D52-9CC0-2E0C1C70A0F2")) ITextColorEffect : public IUnknown
{
    virtual Color GetColor() const noexcept = 0;
};

class TextColorEffect final : public ITextColorEffect
{
public:
    explicit TextColorEffect(Color color) : color_(color) {}

    // ITextColorEffect
    Color GetColor() const noexcept override { return color_; }

    // IUnknown
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG __stdcall AddRef() override;
    ULONG __stdcall Release() override;

private:
    ~TextColorEffect() = default;

private:
    ULONG ref_ = 1;
    Color color_;
};