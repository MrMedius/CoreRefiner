#include "Time.hlsli"

Texture2D tex : register(t0);
SamplerState splr : register(s0);

cbuffer UiBackgroundParamsCBuf : register(b0)
{
    float4 bgColor;
    float4 ringColor;
    float period;
    float thickness;
    float ringCount;
    float aspect;
};

float RingWeight(float dist, float radius, float thick)
{
    float d = abs(dist - radius);
    return saturate(1.0 - d / max(thick, 1e-5));
}

float4 main(float2 tc : Texcoord, float2 tc_Org : OriginalTexcoord) : SV_Target
{
    // Sample the texture to determine if this pixel should be discarded (transparent areas)
    float4 col = tex.Sample(splr, tc_Org);
    if (col.a < 0.1)
        discard;
    
    // Set tc to [-0.5, 0.5] range and apply aspect ratio
    float2 p = tc * float2(aspect, 1.0);
    float dist = length(p);
    if (abs(tc.x) > 0.5 || abs(tc.y) > 0.5)
        discard;
    
    // Max radius is the distance from center to the corner of the rectangle
    const float maxRadius = 0.5 * sqrt(aspect * aspect + 1.0);

    float ringSum = 0.0;
    [loop]
    for (int i = 0; i < (int)ringCount; ++i)
    {
        float phase = (float) i / max(ringCount, 1.0);
        float t = frac(totalTime / max(period, 1e-3) + phase);
        float radius = t * maxRadius;
        ringSum = max(ringSum, RingWeight(dist, radius, thickness));
    }

    col = ringSum > 0.0 ? ringColor : bgColor;
    if (col.a < 0.1)
        discard;

    return col;
}