#include "Transform.hlsli"

struct VSOut
{
    float3 modelPos : TEXCOORD0;
    float4 pos : SV_Position;
};

// pos 是模型空间的顶点位置。外径是 1，躺在 XZ 平面上。
VSOut main(float3 pos : Position)
{
    VSOut output;
    output.modelPos = pos;
    output.pos = mul(float4(pos, 1.0f), modelViewProj);
    return output;
}
