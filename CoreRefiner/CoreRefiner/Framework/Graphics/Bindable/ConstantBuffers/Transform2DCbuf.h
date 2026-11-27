#pragma once
#include "ConstantBuffers.h"
#include "Drawable.h"
#include <DirectXMath.h>

namespace Bind
{
    class Transform2DCbuf : public CloningBindable
    {
    protected:
        struct Transforms
        {
            DirectX::XMMATRIX modelViewProj;
        };
    public:
        Transform2DCbuf(Graphics& gfx, UINT slot = 0u);
        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    private:
        static std::unique_ptr<VertexConstantBuffer<Transforms>> pVcbuf;
        const Drawable* pParent = nullptr;
        UINT vpW = 1;
        UINT vpH = 1;
    };
}