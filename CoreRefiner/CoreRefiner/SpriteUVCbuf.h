#pragma once
#include "ConstantBuffers.h"
#include "Drawable.h"
#include <DirectXMath.h>
#include "Provides.h"

namespace Bind
{
    class SpriteUVCbuf : public CloningBindable
    {
    private:
        using UVData = SpriteUVTag::value_type;
    public:
        SpriteUVCbuf(Graphics& gfx, UINT slot = 2u);
        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    protected:
        void UpdateBindImpl(Graphics& gfx, const UVData& data) noxnd;
        UVData GetUVData() const noexcept;
    private:
        static std::unique_ptr<VertexConstantBuffer<UVData>> pVcbuf;
        const Drawable* pParent = nullptr;
    };
}