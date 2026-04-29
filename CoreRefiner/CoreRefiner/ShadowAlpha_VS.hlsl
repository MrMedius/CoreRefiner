#include "Transform.hlsli"
#include "SpriteUV.hlsli"

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv : Texcoord;
};

VSOut main(float3 pos : Position, float2 uv : Texcoord)
{
    VSOut o;
    o.pos = mul(float4(pos, 1.0f), modelViewProj);
    o.uv = uv * uvScale + uvOffset;
    return o;
}