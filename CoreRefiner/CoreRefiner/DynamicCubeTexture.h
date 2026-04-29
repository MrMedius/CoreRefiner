#pragma once
#include "Bindable.h"
#include "Drawable.h"
#include <memory>
#include <vector>
#include <string>

class Graphics;

namespace Bind
{
    class CubeTexture;

    class DynamicCubeTexture : public CloningBindable
    {
    public:
        DynamicCubeTexture(Graphics& gfx, std::vector<std::string> paths, UINT slot = 0u);

        void SetSlot(UINT s) noexcept { slot = s; }

        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;

    private:
        const Drawable* pParent = nullptr;
        UINT slot = 0u;
        std::vector<std::shared_ptr<Bind::CubeTexture>> frames;
    };
}