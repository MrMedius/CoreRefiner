#include "Transform2DCbuf.h"
#include "Graphics.h"
#include <cassert>

namespace Bind
{
    std::unique_ptr<VertexConstantBuffer<Transform2DCbuf::Transforms>> Transform2DCbuf::pVcbuf;

    Transform2DCbuf::Transform2DCbuf(Graphics& gfx, UINT slot)
        : 
        vpW(gfx.GetWidth()), 
        vpH(gfx.GetHeight())
    {
        if (!pVcbuf)
        {
            pVcbuf = std::make_unique<VertexConstantBuffer<Transforms>>(gfx, slot);
        }
    }

    void Transform2DCbuf::InitializeParentReference(const Drawable& parent) noexcept
    {
        pParent = &parent;
    }

    std::unique_ptr<CloningBindable> Transform2DCbuf::Clone() const noexcept
    {
        return std::make_unique<Transform2DCbuf>(*this);
    }

    void Transform2DCbuf::Bind(Graphics& gfx) noxnd
    {
        using namespace DirectX;
        assert(pParent);

        const XMMATRIX model = pParent->GetTransformXM();
        const XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(0.0f, (float)vpW, (float)vpH, 0.0f, 0.0f, 1.0f);

        const XMMATRIX mvp = model * ortho;

        Transforms tf;
        tf.modelViewProj = XMMatrixTranspose(mvp);

        pVcbuf->Update(gfx, tf);
        pVcbuf->Bind(gfx);
    }
}
