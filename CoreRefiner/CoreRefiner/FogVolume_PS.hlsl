cbuffer FogParams : register(b2)
{
    float4 topColor;
    float4 bottomColor;
    float topY;
    float bottomY;
    float density; // 0.03~0.12
    float stepLen;
    float ditherAmp; // 0.01~0.05
    float padding;
};

struct PSIn
{
    float worldY : TEXCOORD0;
    float4 pos : SV_Position;
};

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float4 main(PSIn i) : SV_Target
{
    float tH = saturate((i.worldY - bottomY) / (topY - bottomY));

    float3 rgb = lerp(bottomColor.rgb, topColor.rgb, tH);

    // gets thinner the higher
    float sigma = density * pow(1.0 - tH, 1.5);

    // The transmittance of each layer in one step is approximatelyÅF
    // alpha = 1 - exp(-sigma * stepLen)
    float a = 1.0 - exp(-sigma * stepLen);

    // screen space dither: breaking up the "regular stripes"
    float n = Hash12(i.pos.xy);
    a = saturate(a + (n - 0.5) * ditherAmp);

    if (a <= 0.001f)
        discard;
    return float4(rgb, a);
}
