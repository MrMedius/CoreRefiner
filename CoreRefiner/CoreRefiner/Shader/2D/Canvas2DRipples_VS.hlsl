#include "Transform2D.hlsli"

struct VSOut
{
    float2 tc : Texcoord;
    float4 pos : SV_Position;
    float2 tc_Org : OriginalTexcoord;
};

VSOut main(float3 pos : Position, float2 tc : Texcoord)
{
    VSOut o;
    o.pos = mul(float4(pos, 1.0f), modelViewProj);
    
    o.tc = (tc - 0.5);
    o.tc_Org = tc;
    
    return o;
}