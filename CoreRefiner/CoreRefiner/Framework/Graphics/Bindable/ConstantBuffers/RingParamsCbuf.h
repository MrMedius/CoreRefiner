#pragma once
#include "ConstantBuffers.h"

namespace Bind
{
    class RingParamsCbuf : public CloningBindable
    {
    public:
        // 写入像素常量缓冲的环参数。字段顺序和原来问父对象拿到的数据一致。
        struct RingData
        {
            // 环的起始角，单位是度。
            float startAngleDeg{ 0.0f };
            // 环的结束角，单位是度。
            float endAngleDeg{ 359.0f };
            // 环画到多少。1 是整环。
            float ratio{ 1.0f };
            // 占位，不参与绘制。
            float padding{ 0.0f };
        };

        RingParamsCbuf(Graphics& gfx, UINT slot = 0u);
        // 记下这一份环参数。data 含起止角和进度。Bind 读这一份。
        void SetRing(const RingData& data) noexcept { ring_ = data; }
        void Bind(Graphics& gfx) noxnd override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    protected:
        void UpdateBindImpl(Graphics& gfx, const RingData& data) noxnd;
    private:
        static std::unique_ptr<PixelConstantBuffer<RingData>> pPcbuf;
        // 当前要写入着色器的环参数。默认从 0 度到 359 度、进度 1。Bind 读它。
        RingData ring_{};
    };
}
