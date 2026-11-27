#include "Transform.hlsli"

struct VSOut
{
    // 模型空间位置。条纹按它划分，球旋转时条纹跟着转。
    float3 modelPos : TEXCOORD0;
    // 视线空间的表面点。摄像机在这个空间的原点。
    float3 viewPos : TEXCOORD1;
    // 视线空间的朝外法线。球的顶点方向就是法线。
    float3 viewNormal : TEXCOORD2;
    float4 pos : SV_Position;
};

// pos 是模型空间的顶点位置。
VSOut main(float3 pos : Position)
{
    VSOut output;
    // modelPos 是模型空间位置。条纹按它划分，球旋转时条纹跟着转。
    output.modelPos = pos;
    // normal 是模型空间的朝外法线。球的顶点方向就是法线。
    float3 normal = normalize(pos);
    output.viewPos = mul(float4(pos, 1.0f), modelView).xyz;
    output.viewNormal = mul(float4(normal, 0.0f), modelView).xyz;
    output.pos = mul(float4(pos, 1.0f), modelViewProj);
    return output;
}
