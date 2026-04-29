#pragma once
#include "ConstantBuffers.h"
#include "Drawable.h"
#include "Provides.h" 

namespace Bind
{
    class RingParamsCbuf : public CloningBindable
    {
    private:
        using RingData = RingParamsTag::value_type;
    public:
        RingParamsCbuf(Graphics& gfx, UINT slot = 0u);
        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    protected:
        void UpdateBindImpl(Graphics& gfx, const RingData& data) noxnd;
        RingData GetRingData() const noexcept;
    private:
        static std::unique_ptr<PixelConstantBuffer<RingData>> pPcbuf;
        const Drawable* pParent = nullptr;
    };
}