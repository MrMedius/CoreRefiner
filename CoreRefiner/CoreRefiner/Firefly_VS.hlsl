#include "Transform.hlsli"

struct VSIn
{
    float3 pos : Position;
    float2 tc : Texcoord;
    float4 col : Color;
};

struct VSOut
{
    float2 tc : TEXCOORD0;
    float fade : TEXCOORD1;
    float4 pos : SV_Position;
};

VSOut main(VSIn i)
{
    VSOut o;
    o.pos = mul(float4(i.pos, 1.0f), modelViewProj);
    o.tc = i.tc;
    o.fade = saturate(i.col.x);
    return o;
}
