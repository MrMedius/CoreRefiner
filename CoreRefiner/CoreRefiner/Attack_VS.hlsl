#include "Transform.hlsli"

// pos 是模型空间的顶点位置。乘上 modelViewProj 得到裁剪空间位置。
float4 main(float3 pos : Position) : SV_Position
{
    return mul(float4(pos, 1.0f), modelViewProj);
}
