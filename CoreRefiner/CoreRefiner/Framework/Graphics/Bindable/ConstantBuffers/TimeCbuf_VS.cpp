#include "TimeCbuf_VS.h"
#include "Graphics.h"

namespace Bind
{
    std::unique_ptr<VertexConstantBuffer<TimeData>> TimeCbuf_VS::pVcbuf;

    TimeCbuf_VS::TimeCbuf_VS(Graphics& gfx, UINT slot)
        : slot_(slot)
    {
        if (!pVcbuf)
            pVcbuf = std::make_unique<VertexConstantBuffer<TimeData>>(gfx, TimeCodex::Get().GetData(), slot);
    }

    void TimeCbuf_VS::Bind(Graphics& gfx) noxnd
    {
        if (!pVcbuf)
            return;
        pVcbuf->Update(gfx, TimeCodex::Get().GetData());
        pVcbuf->Bind(gfx);
    }
}