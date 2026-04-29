Texture2D noiseTex : register(t0);
SamplerState samp : register(s0);

cbuffer ColorBuffer : register(b1)
{
    float4 particleColor;
};

struct PSIn
{
    float2 tc : TEXCOORD0;
    float fade : TEXCOORD1;
    float4 pos : SV_Position;
};

float4 main(PSIn i) : SV_Target
{
    // get noise texture
    float4 tex = noiseTex.Sample(samp, i.tc);

    // 1) Soft edge shapeÅFget the alpha
    float shape = tex.a;

    // 2) Make the edges softer / the center brighter
    shape = pow(saturate(shape), 1.6);

    // 3) Final brightness: Texture shape * Blinking fade
    float intensity = shape * saturate(i.fade);

    // 4) In Additive mode, the output is typically "RGB * intensity", and the alpha value can be set arbitrarily.
    float3 rgb = particleColor.rgb * intensity;

    // 5) White "core" at the center
    float core = pow(saturate(shape), 0.35) * 0.35;
    rgb += core;

    // discard if intensity is too low
    if (intensity < 0.001f)
        discard;

    return float4(rgb, intensity);
}
