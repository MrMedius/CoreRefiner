Texture2D tex : register(t0);
SamplerState splr : register(s0);

cbuffer SliderFillParamsCBuf : register(b0)
{
    float fillAmount;
    float3 _pad;
};

float4 main(float2 tc : Texcoord) : SV_Target
{
    float4 color = tex.Sample(splr, tc);

    if (color.a < 0.1)
        discard;

    if (tc.x > saturate(fillAmount))
        discard;

    return color;
}