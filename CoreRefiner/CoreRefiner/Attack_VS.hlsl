#include "Transform.hlsli"

struct VSOut
{
    // 模型空间位置。条纹按它划分，球旋转时条纹跟着转。
    float3 modelPos : TEXCOORD0;
    float4 pos : SV_Position;
};

// pos 是模型空间的顶点位置。
VSOut main(float3 pos : Position)
{
    VSOut output;
    output.modelPos = pos;
    output.pos = mul(float4(pos, 1.0f), modelViewProj);
    return output;
}
