#pragma once
#include "ConstantBuffers.h"
#include "Bindable.h"
#include "TimeCodex.h"

namespace Bind
{
    class TimeCbuf_VS : public Bindable
    {
    public:
        // slot 是顶点寄存器。默认 b3，和 Time_VS.hlsli 一致。
        // b0 是 Transform，b1 是 VShadow，b2 是 SpriteUV。
        explicit TimeCbuf_VS(Graphics& gfx, UINT slot = 3u);

        void Bind(Graphics& gfx) noxnd override;

    private:
        static std::unique_ptr<VertexConstantBuffer<TimeData>> pVcbuf;
        UINT slot_;
    };
}
