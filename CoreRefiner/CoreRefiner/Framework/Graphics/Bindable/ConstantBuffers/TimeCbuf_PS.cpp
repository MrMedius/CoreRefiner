#include "TimeCbuf_PS.h"
#include "Graphics.h"

namespace Bind
{
    std::unique_ptr<PixelConstantBuffer<TimeData>> TimeCbuf_PS::pPcbuf;

    TimeCbuf_PS::TimeCbuf_PS(Graphics& gfx, UINT slot)
        : slot_(slot)
    {
        if (!pPcbuf)
            pPcbuf = std::make_unique<PixelConstantBuffer<TimeData>>(gfx, TimeCodex::Get().GetData(), slot);
    }

    void TimeCbuf_PS::Bind(Graphics& gfx) noxnd
    {
        if (!pPcbuf)
            return;
        pPcbuf->Update(gfx, TimeCodex::Get().GetData());
        pPcbuf->Bind(gfx);
    }
}