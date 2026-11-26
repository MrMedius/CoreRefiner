Texture2D tex : register(t0);
SamplerState splr : register(s0);

// tc 是画布上的纹理坐标。颜色和透明度都来自这张画布。
float4 main(float2 tc : Texcoord) : SV_Target
{
    return tex.Sample(splr, tc);
}
