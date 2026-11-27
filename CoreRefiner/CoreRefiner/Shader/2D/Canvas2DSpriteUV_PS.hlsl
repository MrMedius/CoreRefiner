Texture2D tex : register(t0);
SamplerState splr : register(s0);

float4 main(float2 tc : Texcoord, float2 reveal : Reveal) : SV_Target
{
    if (tc.x > reveal.x)
        discard;
    if (tc.y > reveal.y)
        discard;

    float4 color = tex.Sample(splr, tc);
    if (color.a < 0.1)
        discard;

    return color;
}
