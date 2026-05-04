Texture2D layerMain : register(t10);
Texture2D layerMini : register(t11);
Texture2D layerHud : register(t12);
SamplerState splr : register(s0);

static float4 Over(float4 dst, float4 src)
{
    float3 rgb = src.rgb * src.a + dst.rgb * (1.0f - src.a);
    float a = src.a + dst.a * (1.0f - src.a);
    return float4(rgb, a);
}

float4 main(float2 uv : Texcoord) : SV_Target
{
    float4 base = layerMain.Sample(splr, uv);

    // Sub-canvas A: minimap viewport in bottom-right UV rect
    float2 miniAnchor = float2(0.72f, 0.62f);
    float2 miniScale = float2(0.26f, 0.34f);
    float2 mu = (uv - miniAnchor) / miniScale;
    float4 miniPx = layerMini.Sample(splr, mu);

    float4 layered = base;
    if (mu.x >= 0.0f && mu.x <= 1.0f && mu.y >= 0.0f && mu.y <= 1.0f)
        layered = Over(layered, miniPx);

    // Sub-canvas B: full-screen hud / vignette-style mask layer
    float4 hudPx = layerHud.Sample(splr, uv);
    layered = Over(layered, hudPx);

    return float4(layered.rgb, layered.a);
}
