#pragma once
#include "ConstantBuffers.h"
#include "Bindable.h"
#include "TimeCodex.h"

namespace Bind
{
    class TimeCbuf_VS : public Bindable
    {
    public:
        explicit TimeCbuf_VS(Graphics& gfx, UINT slot = 0u);

        void Bind(Graphics& gfx) noxnd override;

    private:
        static std::unique_ptr<VertexConstantBuffer<TimeData>> pVcbuf;
        UINT slot_;
    };
}
