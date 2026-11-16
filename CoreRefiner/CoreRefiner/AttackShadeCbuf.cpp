#include "AttackShadeCbuf.h"
#include <cassert>

static_assert(sizeof(Bind::AttackShadeCbuf::ShadeData) == 16);

namespace Bind
{
    AttackShadeCbuf::AttackShadeCbuf(Graphics& gfx, UINT slot)
    {
        // 缓冲只建一次。slot 是像素着色器寄存器号，攻击着色器的 AttackShade 在 b1。
        if (!pPcbuf)
        {
            pPcbuf = std::make_unique<PixelConstantBuffer<ShadeData>>(gfx, ShadeData{}, slot);
        }
    }

    void AttackShadeCbuf::Bind(Graphics& gfx) noxnd
    {
        // shade_ 是这一发的颜色编号。先写入共用缓冲，再绑到像素着色器。
        UpdateBindImpl(gfx, shade_);
    }

    std::unique_ptr<CloningBindable> AttackShadeCbuf::Clone() const noexcept
    {
        return std::make_unique<AttackShadeCbuf>(*this);
    }

    void AttackShadeCbuf::UpdateBindImpl(Graphics& gfx, const ShadeData& data) noxnd
    {
        assert(pPcbuf != nullptr);
        pPcbuf->Update(gfx, data);
        pPcbuf->Bind(gfx);
    }

    std::unique_ptr<PixelConstantBuffer<AttackShadeCbuf::ShadeData>> AttackShadeCbuf::pPcbuf;
}
