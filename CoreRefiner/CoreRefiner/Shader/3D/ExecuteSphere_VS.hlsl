#include "Transform.hlsli"

struct VSOut
{
    float3 viewPos : TEXCOORD0;
    float3 viewNormal : TEXCOORD1;
    float4 pos : SV_Position;
};

// pos 是单位球上的模型位置，方向就是朝外的法线。
// viewPos、viewNormal 在视线空间。摄像机在这个空间的原点。
VSOut main(float3 pos : Position)
{
    VSOut output;
    float3 normal = normalize(pos);
    output.viewPos = mul(float4(pos, 1.0f), modelView).xyz;
    output.viewNormal = mul(float4(normal, 0.0f), modelView).xyz;
    output.pos = mul(float4(pos, 1.0f), modelViewProj);
    return output;
}
