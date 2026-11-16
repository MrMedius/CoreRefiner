#pragma once
#include "ConstantBuffers.h"
#include <DirectXMath.h>

namespace Bind
{
    class SpriteUVCbuf : public CloningBindable
    {
    public:
        // 写入顶点常量缓冲的 UV。字段顺序和原来问父对象拿到的数据一致。
        struct UVData
        {
            // 采样起点。
            DirectX::XMFLOAT2 offset{ 0.0f, 0.0f };
            // 裁切用的缩放。
            DirectX::XMFLOAT2 scale{ 1.0f, 1.0f };
            // 采样窗口。遮罩不改它，仍用 scale 做裁切。
            DirectX::XMFLOAT2 sampleScale{ 1.0f, 1.0f };
            // 占位，不参与采样。
            DirectX::XMFLOAT2 padding{ 0.0f, 0.0f };
        };

        SpriteUVCbuf(Graphics& gfx, UINT slot = 2u);
        // 记下这一份要写入常量缓冲的 UV。data 含偏移、缩放和采样窗口。Bind 读这一份。
        void SetUV(const UVData& data) noexcept { uv_ = data; }
        void Bind(Graphics& gfx) noxnd override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    protected:
        void UpdateBindImpl(Graphics& gfx, const UVData& data) noxnd;
    private:
        static std::unique_ptr<VertexConstantBuffer<UVData>> pVcbuf;
        // 当前要写入着色器的 UV。默认整张图。Bind 读它。
        UVData uv_{};
    };
}
