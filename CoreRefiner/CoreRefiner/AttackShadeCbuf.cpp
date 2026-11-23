#include "AttackShadeCbuf.h"
#include <cassert>

static_assert(sizeof(Bind::AttackShadeCbuf::ShadeData) == 1040);

namespace Bind
{
    AttackShadeCbuf::AttackShadeCbuf(Graphics& gfx, UINT slot)
    {
        // 缓冲只建一次。slot 是像素着色器寄存器号，攻击着色器的 AttackShade 在 b2。
        if (!pPcbuf)
        {
            pPcbuf = std::make_unique<PixelConstantBuffer<ShadeData>>(gfx, ShadeData{}, slot);
        }
    }

    void AttackShadeCbuf::Bind(Graphics& gfx) noxnd
    {
        // uploadCount 是这次实际上传的条数，最多 256。
        constexpr std::size_t kUploadMax = 256u;
        const std::size_t uploadCount = (ids_.size() < kUploadMax) ? ids_.size() : kUploadMax;

        ShadeData data{};
        data.count = static_cast<std::uint32_t>(uploadCount);
        for (std::size_t i = 0; i < uploadCount; ++i)
        {
            data.ids[i] = ids_[i];
        }

        UpdateBindImpl(gfx, data);
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
