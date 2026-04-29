#include "RingParamsCbuf.h"
#include <cassert>

namespace Bind
{
    RingParamsCbuf::RingParamsCbuf(Graphics& gfx, UINT slot)
    {
        if (!pPcbuf)
        {
            RingData init{ 0.f, 360.f, 1.f, 0.f };
            pPcbuf = std::make_unique<PixelConstantBuffer<RingData>>(gfx, init, slot);
        }
    }

    void RingParamsCbuf::Bind(Graphics& gfx) noxnd
    {
        assert(pParent != nullptr);
        UpdateBindImpl(gfx, GetRingData());
    }

    void RingParamsCbuf::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
    }

    std::unique_ptr<CloningBindable> RingParamsCbuf::Clone() const noexcept
    {
        return std::make_unique<RingParamsCbuf>(*this);
    }

    void RingParamsCbuf::UpdateBindImpl(Graphics& gfx, const RingData& data) noxnd
    {
        pPcbuf->Update(gfx, data);
        pPcbuf->Bind(gfx);
    }

    RingParamsCbuf::RingData RingParamsCbuf::GetRingData() const noexcept
    {
        assert(pParent != nullptr);

        const auto rp = TryProvide<RingParamsTag>(pParent);

        return { rp.startAngleDeg, rp.endAngleDeg, rp.ratio, rp.padding };
    }

    std::unique_ptr<PixelConstantBuffer<RingParamsCbuf::RingData>> RingParamsCbuf::pPcbuf;
}
