Texture2D tex : register(t0);
SamplerState splr : register(s0);

float4 main(float2 tc : Texcoord) : SV_Target
{
    float4 color = tex.Sample(splr, tc);
    clip(color.a - 0.1f);

    return 0;
}