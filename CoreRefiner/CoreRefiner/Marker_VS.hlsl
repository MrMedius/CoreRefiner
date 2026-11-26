#include "Transform.hlsli"

struct VSOut
{
    float2 tc : Texcoord;
    float4 pos : SV_Position;
};

// pos 是模型空间的顶点。tc 是画布上的纹理坐标。
VSOut main(float3 pos : Position, float2 tc : Texcoord)
{
    VSOut output;
    output.tc = tc;
    output.pos = mul(float4(pos, 1.0f), modelViewProj);
    return output;
}
