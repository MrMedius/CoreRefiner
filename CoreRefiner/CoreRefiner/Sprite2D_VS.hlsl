#include "Transform2D.hlsli"
#include "SpriteUV.hlsli"

struct VSOut
{
    float2 tc : Texcoord;
    float4 pos : SV_Position;
};

VSOut main(float3 pos : Position, float2 tc : Texcoord)
{
    VSOut o;
    o.pos = mul(float4(pos, 1.0f), modelViewProj);
    
    tc.y = 1.0f - tc.y;
    o.tc = tc * uvScale + uvOffset;
    return o;
}