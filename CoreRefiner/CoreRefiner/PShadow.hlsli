TextureCube smap : register(t3);
SamplerComparisonState ssam : register(s1);

#define PCF_RANGE 1 // PCF kernel taps = PCF_RANGE * 2 + 1
#define PCF_RADIUS_SCALE 1.0f // PCF angular radius scale, adjust the softness
#define CONST_BIAS 0.0001f // Bias applied in "projected depth space"
#define BIAS_DISTANCE_SCALE 0.001f // Additional bias scaled by distance along major axis
//bias += PSHADOW_BIAS_DISTANCE_SCALE / major

#define ZF 100.0f
#define ZN 0.5f
static const float c1 = ZF / (ZF - ZN);
static const float c0 = -ZN * ZF / (ZF - ZN);

float CalculateShadowDepth(const in float4 shadowPos)
{
    // get the major axis
    const float3 m = abs(shadowPos.xyz);
    const float major = max(m.x, max(m.y, m.z));
    // (c1 * major + c0) / major = c1 + c0/major, major must be > 0
    return (c1 * major + c0) / major;
}

float _ComputeShadowBiasDepthSpace(const float major)
{
    // major ~ distance along dominant axis; clamp to avoid blow-up near 0
    const float safeMajor = max(major, ZN);
    return CONST_BIAS + (BIAS_DISTANCE_SCALE / safeMajor);
}

// Build an orthonormal basis around direction n (unit length)
void _BuildBasis(const float3 n, out float3 t, out float3 b)
{
    // pick a stable "up"
    const float3 up = (abs(n.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

float _ShadowPCF_Cube(const float3 dirN, const float refDepth)
{
#if PCF_RANGE <= 0
    return smap.SampleCmpLevelZero(ssam, dirN, refDepth);
#else
    // Estimate a direction-space radius from cube face resolution.
    // TextureCube GetDimensions returns face width/height.
    uint w, h;
    smap.GetDimensions(w, h);

    // Face UV spans roughly [-1,1] before normalization, so texel in that space ~ 2/w.
    const float texel = 2.0f / max(1.0f, (float) w);
    const float radius = texel * PCF_RADIUS_SCALE;

    float3 t, b;
    _BuildBasis(dirN, t, b);

    float sum = 0.0f;
    [unroll]
    for (int x = -PCF_RANGE; x <= PCF_RANGE; ++x)
    {
        [unroll]
        for (int y = -PCF_RANGE; y <= PCF_RANGE; ++y)
        {
            const float3 dirTap = normalize(dirN + (radius * (float) x) * t + (radius * (float) y) * b);
            sum += smap.SampleCmpLevelZero(ssam, dirTap, refDepth);
        }
    }

    const float denom = (float) ((PCF_RANGE * 2 + 1) * (PCF_RANGE * 2 + 1));
    return sum / denom;
#endif
}

float Shadow(const in float4 shadowPos)
{
    const float3 v = shadowPos.xyz;
    const float3 m = abs(v);
    const float major = max(m.x, max(m.y, m.z));

    // Degenerate case: if vector is too small, treat as lit
    if (major <= 1e-6f)
        return 1.0f;

    float refDepth = (c1 * major + c0) / major;

    // Outside shadow map depth range -> lit
    // (If your shadow map generation clamps differently, adjust here)
    if (refDepth <= 0.0f || refDepth >= 1.0f)
        return 1.0f;

    // Apply bias (in depth space)
    refDepth -= _ComputeShadowBiasDepthSpace(major);
    refDepth = saturate(refDepth);

    const float3 dirN = v / major; // normalize(v) but cheaper/stable since major > 0
    return _ShadowPCF_Cube(dirN, refDepth);
}