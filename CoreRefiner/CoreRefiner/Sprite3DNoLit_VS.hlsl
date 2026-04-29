#include "Transform.hlsli"
#include "SpriteUV.hlsli"

struct VSOut
{
    float3 viewPos : Position;
    float2 tc : Texcoord;
    float4 pos : SV_Position;
};

VSOut main(float3 pos : Position, float2 tc : Texcoord)
{
    VSOut vso;
    vso.viewPos = (float3) mul(float4(pos, 1.0f), modelView);
    vso.pos = mul(float4(pos, 1.0f), modelViewProj);
    vso.tc = tc * uvScale + uvOffset;
    return vso;
}