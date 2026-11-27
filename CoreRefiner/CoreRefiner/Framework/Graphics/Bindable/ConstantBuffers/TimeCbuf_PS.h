#pragma once
#include "ConstantBuffers.h"
#include "Bindable.h"
#include "TimeCodex.h"

namespace Bind
{
    class TimeCbuf_PS : public Bindable
    {
    public:
        // slot 是像素寄存器。默认 b1，和 Time_PS.hlsli 一致。
        explicit TimeCbuf_PS(Graphics& gfx, UINT slot = 1u);

        void Bind(Graphics& gfx) noxnd override;

    private:
        static std::unique_ptr<PixelConstantBuffer<TimeData>> pPcbuf;
        UINT slot_;
    };
}