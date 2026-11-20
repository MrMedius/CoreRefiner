#pragma once
#include "ConstantBuffers.h"
#include <cstdint>
#include <vector>

namespace Bind
{
    class AttackShadeCbuf : public CloningBindable
    {
    public:
        // 上传给像素着色器的颜色列表。count 是这次上传的条数。
        // ids 最多 256 条。和攻击像素着色器布局一致，共 1040 字节。
        struct ShadeData
        {
            // 这次上传的条数。0 表示没有条纹。
            std::uint32_t count{ 0 };
            // 补齐到 16 字节，不参与着色。转动用 Time_PS.hlsli 的 totalTime。
            float padding[3]{};
            // 按写入顺序排列的颜色编号。超出 256 的部分不上传。
            std::uint32_t ids[256]{};
        };

        AttackShadeCbuf(Graphics& gfx, UINT slot = 2u);
        // 清掉这一发已经记下的全部颜色编号。
        void ClearColorIds() noexcept { ids_.clear(); }
        // 按写入顺序再追加一个颜色编号。colorId 为 0 是灰，1 是白，2 是黑。
        void AppendColorId(std::uint32_t colorId) { ids_.push_back(colorId); }
        void Bind(Graphics& gfx) noxnd override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    protected:
        void UpdateBindImpl(Graphics& gfx, const ShadeData& data) noxnd;
    private:
        // 所有攻击共用的像素常量缓冲。每发 Bind 前把自己的列表写进去。
        static std::unique_ptr<PixelConstantBuffer<ShadeData>> pPcbuf;
        // 这一发按写入顺序记下的全部颜色编号。着色器只取前 256 个。
        std::vector<std::uint32_t> ids_;
    };
}
