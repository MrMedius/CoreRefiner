#pragma once
#include "ConstantBuffers.h"
#include "Drawable.h"
#include "Provides.h"
#include "CubeTexture.h"
#include <memory>
#include <vector>
#include <string>

class Graphics;

namespace Bind
{
    class SkyboxTransitionCbuf : public CloningBindable
    {
    private:
        using SkyboxData = SkyboxTransitionTag::value_type;
    public:
        SkyboxTransitionCbuf(Graphics& gfx, const std::vector<std::string>& paths, UINT slotFromTex = 0u, UINT slotToTex = 1u, UINT slotPSCBuf = 2u);
        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    private:
        SkyboxData GetData() const noexcept;
        void UpdateBindImpl(Graphics& gfx, const SkyboxData& data) noxnd;
    private:
        static std::unique_ptr<PixelConstantBuffer<SkyboxData>> pPcbuf;
        std::vector<std::shared_ptr<CubeTexture>> cubes;
        UINT slotFromTex;
        UINT slotToTex;
        const Drawable* pParent = nullptr;
    };
}
