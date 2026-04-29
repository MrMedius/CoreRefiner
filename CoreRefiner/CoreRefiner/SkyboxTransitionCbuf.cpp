#include "SkyboxTransitionCbuf.h"
#include <cassert>

namespace Bind
{
    SkyboxTransitionCbuf::SkyboxTransitionCbuf(Graphics& gfx, const std::vector<std::string>& paths, UINT slotFromTex, UINT slotToTex, UINT slotPSCBuf)
        :
        slotFromTex(slotFromTex),
        slotToTex(slotToTex)
    {
        if (!pPcbuf)
        {
            pPcbuf = std::make_unique<PixelConstantBuffer<SkyboxData>>(gfx, slotPSCBuf);
        }

        cubes.reserve(paths.size());
        for (auto& p : paths)
        {
            cubes.push_back(CubeTexture::Resolve(gfx, p));
        }

        assert(!cubes.empty());
    }

    void SkyboxTransitionCbuf::Bind(Graphics& gfx) noxnd
    {
        assert(pParent != nullptr);

        const auto data = GetData();

        const int n = (int)cubes.size();
        if (n <= 0) return;

        int fromIdx = data.fromIdx % n;
        if (fromIdx < 0) fromIdx += n;

        int toIdx = data.toIdx % n;
        if (toIdx < 0) toIdx += n;

        cubes[(size_t)fromIdx]->SetSlot(slotFromTex);
        cubes[(size_t)fromIdx]->Bind(gfx);
        cubes[(size_t)toIdx]->SetSlot(slotToTex);
        cubes[(size_t)toIdx]->Bind(gfx);

        UpdateBindImpl(gfx, data);
    }

    void SkyboxTransitionCbuf::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
    }

    std::unique_ptr<CloningBindable> SkyboxTransitionCbuf::Clone() const noexcept
    {
        return std::make_unique<SkyboxTransitionCbuf>(*this);
    }

    SkyboxTransitionCbuf::SkyboxData SkyboxTransitionCbuf::GetData() const noexcept
    {
        assert(pParent != nullptr);
        return TryProvide<SkyboxTransitionTag>(pParent);
    }

    void SkyboxTransitionCbuf::UpdateBindImpl(Graphics& gfx, const SkyboxData& data) noxnd
    {
        pPcbuf->Update(gfx, data);
        pPcbuf->Bind(gfx);
    }

    std::unique_ptr<PixelConstantBuffer<SkyboxTransitionCbuf::SkyboxData>> SkyboxTransitionCbuf::pPcbuf;
}
