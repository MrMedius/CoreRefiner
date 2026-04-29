#include "FieldTransitionCbuf.h"
#include <cassert>

namespace Bind
{
    FieldTransitionCbuf::FieldTransitionCbuf(Graphics& gfx, UINT slot)
    {
        if (!pPcbuf)
        {
            pPcbuf = std::make_unique<PixelConstantBuffer<FieldData>>(gfx, slot);
        }
    }

    void FieldTransitionCbuf::Bind(Graphics& gfx) noxnd
    {
        assert(pParent != nullptr);

        const auto data = GetFieldData();
        UpdateBindImpl(gfx, data);
    }

    void FieldTransitionCbuf::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
    }

    std::unique_ptr<CloningBindable> FieldTransitionCbuf::Clone() const noexcept
    {
        return std::make_unique<FieldTransitionCbuf>(*this);
    }

    void FieldTransitionCbuf::UpdateBindImpl(Graphics& gfx, const FieldData& data) noxnd
    {
        pPcbuf->Update(gfx, data);
        pPcbuf->Bind(gfx);
    }

    FieldTransitionCbuf::FieldData FieldTransitionCbuf::GetFieldData() const noexcept
    {
        assert(pParent != nullptr);

        return TryProvide<FieldTransitionTag>(pParent);
    }

    std::unique_ptr<PixelConstantBuffer<FieldTransitionCbuf::FieldData>> FieldTransitionCbuf::pPcbuf;
}
