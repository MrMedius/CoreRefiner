Texture2D diffuseTex : register(t0);
SamplerState samp : register(s0);

#define SHADOW_ALPHA_THRESHOLD 0.5f

void main(float4 pos : SV_Position, float2 uv : Texcoord)
{
    const float a = diffuseTex.Sample(samp, uv).a;
    clip(a - SHADOW_ALPHA_THRESHOLD);
}