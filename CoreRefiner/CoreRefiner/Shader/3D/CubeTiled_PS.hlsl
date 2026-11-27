#include "ShaderOps.hlsli"
#include "LightVectorData.hlsli"

#include "PointLight.hlsli"
#include "PShadow.hlsli"

#include "MathHelpers.hlsli"

cbuffer ObjectCBuf : register(b1)
{
    float3 specularColor;
    float specularWeight;
    float specularGloss;
};

cbuffer TransitionCBuf : register(b2)
{
    // core
    int trState; // 0 none, 1 expand, 2 recover
    int trMode; // 0 normal, 1 burning, 2 purifying
    // parameters
    float trRadius;
    float trSoftness; // blend width
    float3 trCenter;
    float opacity;
    // RGB
    float3 colorFrom;
    float pad0;
    float3 colorTo;
    float pad1;
    // noise
    float noiseScale; // noise uv scale
    float noiseAmp; // noise displaces edge (burning)
};

Texture2D tex : register(t0);
Texture2D noiseTex : register(t1);

SamplerState splr : register(s0);

// mode 3 helpers
float HexDist(float2 p)
{
    p = abs(p);
    float c = dot(p, normalize(float2(1.0, 1.73))); // float(1.0, Å„3)
    c = max(c, p.x);
    return c;
}
float4 GetHexGrid(float2 uv)
{
    float2 r = float2(1.0, 1.73); // float(1.0, Å„3)
    float2 h = r * 0.5;

    float2 a = safe_mod(uv, r) - h;
    float2 b = safe_mod(uv - h, r) - h;

    float2 gv = dot(a, a) < dot(b, b) ? a : b;
    float2 id = uv - gv;

    return float4(gv.x, gv.y, id.x, id.y);
}


float4 main(
    float3 viewFragPos : Position,
    float3 viewNormal  : Normal,
    float2 tc          : Texcoord,
    float4 spos        : ShadowPosition,
    float3 worldPos    : WorldPos) : SV_Target
{
    // -------------------------
    // lighting
    // -------------------------
    float3 diffuse;
    float3 specular;
    // shadow map test
    const float shadowLevel = Shadow(spos);
    if (shadowLevel != 0.0f)
    {
        // renormalize interpolated normal
        viewNormal = normalize(viewNormal);
	    // fragment to light vector data
        const LightVectorData lv = CalculateLightVectorData(viewLightPos, viewFragPos);
	    // attenuation
        const float att = Attenuate(attConst, attLin, attQuad, lv.distToL);
	    // diffuse
        diffuse = Diffuse(diffuseColor, diffuseIntensity, att, lv.dirToL, viewNormal);
	    // specular
        specular = Speculate(diffuseColor * diffuseIntensity * specularColor, specularWeight, viewNormal, lv.vToL, viewFragPos, att, specularGloss);
        // scale by shadow level
        diffuse *= shadowLevel;
        specular *= shadowLevel;
    }
    else
    {
        diffuse = specular = 0.0f;
    }

    // base texture
    float3 baseRaw = tex.Sample(splr, tc).rgb;

    // State none
    if (trState == 0)
    {
        float3 lit0 = saturate((diffuse + ambient) * baseRaw + specular);
        return float4(lit0, opacity);
    }

    // common distance (world XZ)
    const float dist = distance(worldPos.xz, trCenter.xz);

    // -------------------------
    // per-mode outputs
    // -------------------------
    float3 outTex = baseRaw;
    float alpha = opacity;

    // =========================================================
    // MODE 0: Normal (clean radial smoothstep)
    // =========================================================
    if (trMode == 0)
    {
        float blend;
        if (trState == 1) // Expand: inside -> colorTo
        {
            blend = 1.0f - smoothstep(trRadius - trSoftness, trRadius, dist);
            outTex *= lerp(colorFrom, colorTo, blend);
        }
        else // Recover: inside -> back to colorFrom
        {
            blend = smoothstep(trRadius - trSoftness, trRadius, dist);
            outTex *= lerp(colorTo, colorFrom, blend);
        }
    }

    // =========================================================
    // MODE 1: Burning (noisy boundary + ember ring)
    // =========================================================
    else if (trMode == 1)
    {
        float ns = max(noiseScale, 0.0005f); // Noise frequency (bigger -> finer crack)
        float band = max(trSoftness * 4.0f, trRadius * 1.0f); // Wide influence band around the boundary

        // boundary distance: 0 at boundary, positive outside, negative inside
        float d = dist - trRadius;

        float mask = 1.0f - saturate(abs(d) / band);
        mask = mask * mask;

        // Noise amplitude
        float ampBase = trRadius * 0.20f + trSoftness * 6.0f;
        ampBase = min(ampBase, noiseAmp);
        ampBase = max(ampBase, trSoftness * 1.0f);

        float n = noiseTex.Sample(splr, worldPos.xz * ns).r;
        n = n * 2.0f - 1.0f;

        float noisyDist = dist + n * (ampBase * mask);

        float blend;
        if (trState == 1) // Expand
        {
            blend = 1.0f - smoothstep(trRadius - trSoftness, trRadius, noisyDist);
            outTex *= lerp(colorFrom, colorTo, blend);
        }
        else // Recover
        {
            blend = smoothstep(trRadius - trSoftness, trRadius, noisyDist);
            outTex *= lerp(colorTo, colorFrom, blend);
        }

        // Ember edge
        float edgeWidth = max(trSoftness * 1.0f, 1.0f);

        float edge =
        smoothstep(trRadius - edgeWidth, trRadius, noisyDist) -
        smoothstep(trRadius, trRadius + edgeWidth, noisyDist);

        edge = saturate(edge);

        float3 ember = float3(1.0f, 0.35f, 0.05f) * 2.0f;
        outTex += ember * edge;
    }



    // =========================================================
    // MODE 2: Purifying (wavy boundary + ripple band + sparkle)
    // =========================================================
    else if (trMode == 2)
    {
        float2 dir = worldPos.xz - trCenter.xz;
        float ang = atan2(dir.y, dir.x);

        float waveAmp = clamp(trRadius * 0.10f, trSoftness * 0.8f, 6.0f);
        float waveAmp2 = clamp(trRadius * 0.05f, trSoftness * 0.5f, 3.0f);

        float waveShape = 0.0f;
        waveShape += sin(ang * 6.0f + trRadius * 0.02f) * waveAmp;
        waveShape += cos(ang * 15.0f - trRadius * 0.05f) * waveAmp2;

        float distWavy = dist + waveShape;

        float blend;
        if (trState == 1) // Expand
        {
            blend = 1.0f - smoothstep(trRadius - trSoftness, trRadius, distWavy);
            outTex *= lerp(colorFrom, colorTo, blend);
        }
        else // Recover
        {
            blend = smoothstep(trRadius - trSoftness, trRadius, distWavy);
            outTex *= lerp(colorTo, colorFrom, blend);
        }

        // proportional to radius
        float rippleWidth = clamp(trRadius * 0.18f, 6.0f, 18.0f);
        rippleWidth = max(rippleWidth, trSoftness * 2.0f);

        if (distWavy < trRadius && distWavy > trRadius - rippleWidth)
        {
            float rippleMask = 1.0f - ((trRadius - distWavy) / rippleWidth);

            float surfaceWave = sin(worldPos.x * 0.05f + worldPos.z * 0.05f + trRadius * 0.1f);
            rippleMask *= (0.7f + 0.3f * surfaceWave);

            float2 sparkleUV = worldPos.xz * 0.02f + float2(trRadius * 0.01f, trRadius * 0.02f);
            float n = noiseTex.Sample(splr, sparkleUV).r;
            float sparkle = step(0.75f, n) * rippleMask;

            float3 waterColor = float3(0.0f, 0.7f, 1.0f);
            float3 sparkleColor = float3(1.0f, 1.0f, 1.0f);

            outTex += (waterColor * 0.6f * rippleMask) + (sparkleColor * sparkle);
        }
    }

    // =========================================================
    // MODE 3: Digital (hex construction + glow border + scan/glitch)
    // =========================================================
    else if (trMode == 3)
    {
        float worldScale = 4.0f;

        float2 p = worldPos.xz * worldScale;
        float2 c = trCenter.xz * worldScale;

        float R = trRadius * worldScale;
        float S = max(trSoftness * worldScale, 0.001f);

        // hex grid
        float hexScale = 10.0f;
        float2 hexUV = p / hexScale;
        float4 hexInfo = GetHexGrid(hexUV);

        float hexRand = rand(hexInfo.zw);

        float noiseRand = noiseTex.Sample(
        splr,
        p * 0.005f + float2(R * 0.001f, 0.0f)).r;

        float finalRand = lerp(hexRand, noiseRand, 0.6f);

        // cell center distance in the SAME scaled space
        float gridDist = distance(hexInfo.zw * hexScale, c);

        // randomized construction threshold
        float offsetAmp = 300.0f; // scaled-space amplitude
        float randomOffset = (finalRand - 0.5f) * offsetAmp; // [-0.5A, +0.5A]

        // avoid early appearance
        float earlyClamp = offsetAmp * 0.35f;
        float threshold = max(gridDist + randomOffset, -earlyClamp);

        // check wheather the Hex is built
        float insideHard = step(threshold, R); // 1 inside, 0 outside

        // soft edge
        float softBand = max(S * 0.35f, 0.5f); // smaller => more jumpy / larger => more blurry
        float insideEdge = smoothstep(R - softBand, R, threshold); // 0 inside, 1 near/outside
        float insideSoft = insideHard * (1.0f - insideEdge);

        float3 tint = lerp(colorFrom, colorTo, insideSoft);
        outTex *= tint;

        // Glow border (base on threshold/R)
        float edgeWidth = 100.0f; // adjust the size of the border
        if (insideHard > 0.01f && (R - threshold) < edgeWidth)
        {
            float hDist = HexDist(hexInfo.xy);
            float border =
            smoothstep(0.40f, 0.45f, hDist) +
            smoothstep(0.48f, 0.50f, hDist) * 2.0f;

            float scanline =
            sin(worldPos.y * 0.8f + R * 0.2f) *
            sin(worldPos.z * 0.1f);

            float scanNoise = noiseTex.Sample(
            splr,
            float2(0.0f, worldPos.y * 0.05f + R * 0.01f)
        ).r;

            float glitch = step(0.8f, scanNoise) * 2.0f;

            float3 cyan = float3(0.0f, 1.0f, 1.0f);
            float3 yellow = float3(1.0f, 0.8f, 0.2f);
            float3 emitColor = lerp(cyan, yellow, hexRand);

            float intensity = border * 1.5f + glitch + (scanline * 0.5f);

            // get brighter closer to the edge
            float fade = pow(saturate(1.0f - ((R - threshold) / edgeWidth)), 2.0f);

            outTex += emitColor * intensity * fade;
        }
    }

    // final lit
    float3 lit = saturate((diffuse + ambient) * outTex + specular);
    return float4(lit, alpha);
}