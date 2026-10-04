#include "SpriteUVCbuf.h"
#include <cassert>

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
        assert(pParent != nullptr);
        UpdateBindImpl(gfx, GetUVData());
    }

    void SpriteUVCbuf::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
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

    SpriteUVCbuf::UVData SpriteUVCbuf::GetUVData() const noexcept
    {
        assert(pParent != nullptr);

        const auto uv = TryProvide<SpriteUVTag>(pParent);

        return { uv.offset, uv.scale, uv.sampleScale, uv.padding };
    }

    std::unique_ptr<VertexConstantBuffer<SpriteUVCbuf::UVData>> SpriteUVCbuf::pVcbuf;
}