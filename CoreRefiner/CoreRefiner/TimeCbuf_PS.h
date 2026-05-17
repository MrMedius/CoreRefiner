#pragma once
#include "ConstantBuffers.h"
#include "Bindable.h"
#include "TimeCodex.h"

namespace Bind
{
    class TimeCbuf_PS : public Bindable
    {
    public:
        explicit TimeCbuf_PS(Graphics& gfx, UINT slot = 1u);

        void Bind(Graphics& gfx) noxnd override;

    private:
        static std::unique_ptr<PixelConstantBuffer<TimeData>> pPcbuf;
        UINT slot_;
    };
}