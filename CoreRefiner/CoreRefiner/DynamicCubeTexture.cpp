#include "DynamicCubeTexture.h"
#include "CubeTexture.h"
#include "Provides.h"
#include <cassert>

namespace Bind
{
    DynamicCubeTexture::DynamicCubeTexture(Graphics& gfx, std::vector<std::string> paths, UINT slot)
        : slot(slot)
    {
        frames.reserve(paths.size());
        for (auto& f : paths)
        {
            frames.push_back(Bind::CubeTexture::Resolve(gfx, f, slot));
        }
        assert(!frames.empty());
    }

    void DynamicCubeTexture::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
    }

    std::unique_ptr<CloningBindable> DynamicCubeTexture::Clone() const noexcept
    {
        return std::make_unique<DynamicCubeTexture>(*this);
    }

    void DynamicCubeTexture::Bind(Graphics& gfx) noxnd
    {
        assert(pParent != nullptr);

        int idx = TryProvide<DynamicCubeTextureTag>(pParent);

        const int n = (int)frames.size();
        if (n <= 0) return;

        idx %= n;
        if (idx < 0) idx += n;

        auto& tex = frames[(size_t)idx];
        tex->SetSlot(slot);
        tex->Bind(gfx);
    }
}