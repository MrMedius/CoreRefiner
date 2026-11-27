Texture2D tex : register(t0);
SamplerState splr : register(s0);

// 贴图是黑色菱形。透明的地方丢掉，剩下的颜色再乘上这颗自己的透明度。

float4 main(float2 tc : Texcoord, float fade : Fade) : SV_Target
{
    const float4 color = tex.Sample(splr, tc);
    clip(color.a - 0.1f);
    return float4(color.rgb, color.a * fade);
}
