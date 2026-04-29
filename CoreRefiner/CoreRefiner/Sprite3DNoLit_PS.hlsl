Texture2D tex : register(t0);

SamplerState splr : register(s0);

float4 main(
        float3 viewFragPos : Position,
        float2 tc : Texcoord,
        bool isFrontFace : SV_IsFrontFace
) : SV_Target
{
    // alpha test
    const float4 color = tex.Sample(splr, tc);
    clip(color.a - 0.1f);
 
	// final color
    return color;
}