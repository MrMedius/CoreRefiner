cbuffer TransitionCBuf : register(b2)
{
    // core
    int isCollapsing; // 0 = normal, 1 = collapse
    float trRadius;
    float trSoftness;
    // Tutorial
    float collapseTime;
    float vortexStrength;
    float rippleCount;
    float rainbowIntensity;
    float totalTime;
    float2 numTiles;
    float2 pad;
};

Texture2D texNoise : register(t1);
SamplerState splr  : register(s0);

// Helpers
float3 HSVtoRGB(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0 - abs(fmod(h * 6.0, 2.0) - 1.0));
    float m = v - c;
    float3 rgb;
    if      (h < 1.0/6.0) rgb = float3(c, x, 0.0);
    else if (h < 2.0/6.0) rgb = float3(x, c, 0.0);
    else if (h < 3.0/6.0) rgb = float3(0.0, c, x);
    else if (h < 4.0/6.0) rgb = float3(0.0, x, c);
    else if (h < 5.0/6.0) rgb = float3(x, 0.0, c);
    else                   rgb = float3(c, 0.0, x);
    return rgb + m;
}

float GridLine(float coord, float lineWidth, float glowWidth)
{
    float d = abs(frac(coord) - 0.5) * 2.0;
    d = 1.0 - d;
    float coreLine = smoothstep(1.0 - lineWidth, 1.0, d);
    float glow     = pow(smoothstep(1.0 - glowWidth, 1.0, d), 2.0);
    return coreLine * 1.2 + glow * 0.6;
}

float warpLines(float2 uv, float time)
{
    float line1 = sin(uv.y * 30.0 + time * 2.0 + sin(uv.x * 10.0 + time) * 2.0);
    float line2 = sin(uv.x * 20.0 - time * 1.5 + cos(uv.y * 8.0 + time * 0.7) * 3.0);
    float line3 = sin((uv.x + uv.y) * 15.0 + time * 0.8);
    float v = abs(line1) * abs(line2) + abs(line3) * 0.3;
    return pow(saturate(1.0 - v), 8.0);
}

float WaveVisibility(float2 worldXZ, float2 playerXZ, float time)
{
    float dist  = length(worldXZ - playerXZ);
    float wave1 = sin(dist * 0.04 - time * 2.5) * 0.5 + 0.5;
    float wave2 = sin(dist * 0.03 - time * 1.8 + 2.0) * 0.5 + 0.5;
    float wave3 = sin(dist * 0.06 - time * 3.2 + 4.5) * 0.5 + 0.5;
    float combined = wave1 * 0.45 + wave2 * 0.35 + wave3 * 0.20;
    return smoothstep(0.15, 0.65, combined);
}

float3 CalculateNebula(float2 worldXZ, float time)
{
    float2 baseUV = worldXZ * 0.04;
    float2 nuv1 = baseUV * 1.5 + float2(time * 0.02, time * 0.015);
    float2 nuv2 = baseUV * 3.0 + float2(-time * 0.03, time * 0.01);
    float2 nuv3 = baseUV * 0.8 + float2(time * 0.01, -time * 0.025);
    float n1 = texNoise.Sample(splr, nuv1).r;
    float n2 = texNoise.Sample(splr, nuv2).r;
    float n3 = texNoise.Sample(splr, nuv3).r;
    float nebula = smoothstep(0.3, 0.7, n1 * 0.5 + n2 * 0.3 + n3 * 0.2);
    float nebulaHue = frac(time * 0.03 + nebula * 0.3);
    return HSVtoRGB(nebulaHue, 0.7, 0.4) * nebula * 0.5;
}

float3 CalculateWarpLines(float2 worldXZ, float time)
{
    float2 lineUV = worldXZ * 0.06;
    float lines = warpLines(lineUV, time);
    float  lineHue = frac(time * 0.05 + lineUV.y * 0.5);
    return HSVtoRGB(lineHue, 0.5, 1.0) * lines * 0.25;
}

float3 CalculateCenterGlow(float2 worldXZ, float2 centerXZ, float time)
{
    float dist          = length(worldXZ - centerXZ);
    float normalizedDist = dist / 25.0;
    float centerGlow = exp(-normalizedDist * normalizedDist * 8.0);
    float glowHue       = frac(time * 0.12);
    float3 glowColor    = HSVtoRGB(glowHue, 0.4, 1.0) * centerGlow * 0.3;
    float breathe       = sin(time * 1.5) * 0.3 + 0.7;
    return glowColor * breathe;
}

// ----------------------------------------------------------------
// Main
// ----------------------------------------------------------------
float4 main(
    float3 viewFragPos : Position,
    float3 viewNormal  : Normal,
    float2 tc          : Texcoord,
    float4 spos        : ShadowPosition,
    float3 worldPos    : WorldPos) : SV_Target
{
    float t = totalTime;

    // Center
    float3 center = (0.0, 0.0, 0.0);
    
    // Grid UV
    float gridCountX = max(numTiles.x * 0.5, 8.0);
    float gridCountZ = max(numTiles.y * 0.5, 8.0);
    float2 gridUV = float2(tc.x * gridCountX, tc.y * gridCountZ);

    // Scrolling animation
    float2 animatedUV = gridUV + float2(t * 0.15, t * 0.08);

    // Grid Intensity
    float gridX = GridLine(animatedUV.x, 0.14, 0.40);
    float gridZ = GridLine(animatedUV.y, 0.14, 0.40);
    float gridCross     = gridX * gridZ;
    float gridIntensity = max(gridX, gridZ) + gridCross * 0.8;

    // Second layer (sparse background mesh)
    float2 gridUV2 = float2(tc.x * gridCountX * 0.5, tc.y * gridCountZ * 0.5);
    gridUV2 += float2(-t * 0.05, t * 0.03);
    float gridX2 = GridLine(gridUV2.x, 0.10, 0.25);
    float gridZ2 = GridLine(gridUV2.y, 0.10, 0.25);
    gridIntensity += max(gridX2, gridZ2) * 0.25;

    // Wave visibility masking (player position driven)
    float waveVis = 0.6 + sin(length(worldPos.xz - center.xz) * 0.3 - t * 2.0) * 0.4;
    waveVis = saturate(waveVis);
    gridIntensity *= waveVis;

    // Scanlines
    float scanline   = sin(tc.y * gridCountZ * 6.2832 + t * 5.0) * 0.5 + 0.5;
    float scanEffect = pow(scanline, 8.0) * 0.12;
    gridIntensity += scanEffect * waveVis;

    // Fade-out Edges
    float edgeFade = 0.8;
    gridIntensity *= edgeFade;

    // Mask When Collapse
    if (isCollapsing == 1 && trRadius > 0.01)
    {
        float3 dv = worldPos - center;
        float  dist = length(float2(dv.x, dv.z));
        float  collapseMask = smoothstep(trRadius - trSoftness, trRadius + trSoftness * 0.5, dist);
        gridIntensity *= collapseMask;

        float borderDist2 = abs(dist - trRadius);
        float borderGlow  = exp(-borderDist2 * borderDist2 / (trSoftness * trSoftness * 2.0));
        gridIntensity += borderGlow * 2.0 * collapseMask;
    }

    // Additional effects: Nebula + Warp Lines + Central Glow
    float3 nebulaColor     = CalculateNebula(worldPos.xz, t);
    float3 warpLineColor   = CalculateWarpLines(worldPos.xz, t);
    float3 centerGlowColor = CalculateCenterGlow(worldPos.xz, center.xz, t);

    float effectMask = waveVis * edgeFade;
    if (isCollapsing == 3 && trRadius > 0.01)
    {
        float3 dv = worldPos - center;
        float  dist = length(float2(dv.x, dv.z));
        effectMask *= smoothstep(trRadius - trSoftness, trRadius + trSoftness * 0.5, dist);
    }

    float3 additionalEffects = (nebulaColor + warpLineColor + centerGlowColor) * effectMask;

    // Final
    float3 gridColor      = float3(0.3, 0.7, 1.0);
    float3 gridFinalColor = gridColor * gridIntensity;
    float3 finalColor = gridFinalColor * 0.5 + additionalEffects * 0.5;

    float finalIntensity = max(max(finalColor.r, finalColor.g), finalColor.b);
    if (finalIntensity < 0.005)
        discard;

    return float4(finalColor, finalIntensity);
}