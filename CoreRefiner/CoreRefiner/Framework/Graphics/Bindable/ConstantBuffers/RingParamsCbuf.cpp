#include "RingParamsCbuf.h"

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
        // ring_ 是拥有者写入的起止角和进度。
        UpdateBindImpl(gfx, ring_);
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

    std::unique_ptr<PixelConstantBuffer<RingParamsCbuf::RingData>> RingParamsCbuf::pPcbuf;
}
