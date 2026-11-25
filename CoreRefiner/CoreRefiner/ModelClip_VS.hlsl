#include "Transform.hlsli"

struct VSOut
{
    float3 worldPos : TEXCOORD0;
    float4 pos : SV_Position;
};

// pos 是模型空间的顶点。世界位置交给像素着色器做裁切。
VSOut main(float3 pos : Position)
{
    VSOut output;
    output.worldPos = mul(float4(pos, 1.0f), model).xyz;
    output.pos = mul(float4(pos, 1.0f), modelViewProj);
    return output;
}
