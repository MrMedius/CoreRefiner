#pragma once
#include "ConstantBuffers.h"
#include <cstdint>

namespace Bind
{
    class AttackShadeCbuf : public CloningBindable
    {
    public:
        // 写入像素常量缓冲的颜色。布局和 Attack_PS.hlsl 的 AttackShade 一致，共 16 字节。
        struct ShadeData
        {
            // 颜色编号。0 是灰，1 是白，超出范围由着色器当作灰。
            std::uint32_t colorId{ 0 };
            // 补齐到 16 字节，不参与着色。
            float padding[3]{};
        };

        AttackShadeCbuf(Graphics& gfx, UINT slot = 1u);
        // 记下这一发的颜色编号。colorId 为 0 是灰，1 是白。
        void SetColorId(std::uint32_t colorId) noexcept { shade_.colorId = colorId; }
        void Bind(Graphics& gfx) noxnd override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    protected:
        void UpdateBindImpl(Graphics& gfx, const ShadeData& data) noxnd;
    private:
        // 所有攻击共用的像素常量缓冲。每发 Bind 前把自己的 shade_ 写进去。
        static std::unique_ptr<PixelConstantBuffer<ShadeData>> pPcbuf;
        // 这一发要写入着色器的颜色。默认 0，也就是灰。
        ShadeData shade_{};
    };
}
