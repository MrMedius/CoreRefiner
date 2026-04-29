#include "TutorialTransitionCbuf.h"
#include <cassert>

namespace Bind
{
    TutorialTransitionCbuf::TutorialTransitionCbuf(Graphics& gfx, UINT slot)
    {
        if (!pPcbuf)
        {
            pPcbuf = std::make_unique<PixelConstantBuffer<TutData>>(gfx, slot);
        }
    }

    void TutorialTransitionCbuf::Bind(Graphics& gfx) noxnd
    {
        assert(pParent != nullptr);

        const auto data = GetTutData();
        UpdateBindImpl(gfx, data);
    }

    void TutorialTransitionCbuf::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
    }

    std::unique_ptr<CloningBindable> TutorialTransitionCbuf::Clone() const noexcept
    {
        return std::make_unique<TutorialTransitionCbuf>(*this);
    }

    void TutorialTransitionCbuf::UpdateBindImpl(Graphics& gfx, const TutData& data) noxnd
    {
        pPcbuf->Update(gfx, data);
        pPcbuf->Bind(gfx);
    }

    TutorialTransitionCbuf::TutData TutorialTransitionCbuf::GetTutData() const noexcept
    {
        assert(pParent != nullptr);

        return TryProvide<TutorialTransitionTag>(pParent);
    }

    std::unique_ptr<PixelConstantBuffer<TutorialTransitionCbuf::TutData>> TutorialTransitionCbuf::pPcbuf;
}
