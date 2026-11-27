#include "Transform.hlsli"

struct VSOut
{
    float worldY : TEXCOORD0;
    float4 pos : SV_Position;
};

VSOut main(float3 pos : Position)
{
    VSOut o;
    float4 wpos = mul(float4(pos, 1.0f), model);
    o.worldY = wpos.y;
    o.pos = mul(float4(pos, 1.0f), modelViewProj);
    return o;
}
