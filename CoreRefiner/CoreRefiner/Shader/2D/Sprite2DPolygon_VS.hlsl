#include "Transform2D.hlsli"
#include "SpriteUV.hlsli"

struct VSOut
{
    float2 tc : Texcoord;
    float4 pos : SV_Position;
    
    float2 uv : TexcoordPure;
};

VSOut main(float3 pos : Position, float2 tc : Texcoord)
{
    VSOut o;
    o.pos = mul(float4(pos, 1.0f), modelViewProj);

    float2 tcFlip = tc;
    tcFlip.y = 1.0f - tcFlip.y;

    o.uv = tcFlip;
    o.tc = tcFlip * uvScale + uvOffset;
    return o;
}