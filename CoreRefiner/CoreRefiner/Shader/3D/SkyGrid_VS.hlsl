#include "Transform.hlsli"

struct VSIn
{
    float3 pos : Position;
    float2 uv : Texcoord;
    float3 normal : Normal;
};

struct VSOut
{
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float4 pos : SV_Position;
};

VSOut main(VSIn i)
{
    VSOut o;
    float4 wpos = mul(float4(i.pos, 1.0f), model);
    o.worldPos = wpos.xyz;

    o.pos = mul(float4(i.pos, 1.0f), modelViewProj);

    o.uv = i.uv;
    o.normal = mul(i.normal, (float3x3) model);

    return o;
}