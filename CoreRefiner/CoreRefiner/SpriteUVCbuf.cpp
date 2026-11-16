#include "SpriteUVCbuf.h"

namespace Bind
{
    SpriteUVCbuf::SpriteUVCbuf(Graphics& gfx, UINT slot)
    {
        if (!pVcbuf)
        {
            pVcbuf = std::make_unique<VertexConstantBuffer<UVData>>(gfx, slot);
        }
    }

    void SpriteUVCbuf::Bind(Graphics& gfx) noxnd
    {
        // uv_ 是拥有者写入的采样起点、裁切缩放和采样窗口。
        UpdateBindImpl(gfx, uv_);
    }

    std::unique_ptr<CloningBindable> SpriteUVCbuf::Clone() const noexcept
    {
        return std::make_unique<SpriteUVCbuf>(*this);
    }

    void SpriteUVCbuf::UpdateBindImpl(Graphics& gfx, const UVData& data) noxnd
    {
        pVcbuf->Update(gfx, data);
        pVcbuf->Bind(gfx);
    }

    std::unique_ptr<VertexConstantBuffer<SpriteUVCbuf::UVData>> SpriteUVCbuf::pVcbuf;
}
