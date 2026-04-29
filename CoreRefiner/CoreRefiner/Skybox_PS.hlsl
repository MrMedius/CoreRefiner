//TextureCube tex : register(t0);
//SamplerState sam : register(s0);

//float4 main(float3 worldPos : Position) : SV_TARGET
//{
//    return tex.Sample(sam, worldPos);
//}




TextureCube texFrom : register(t0);
TextureCube texTo : register(t1);
Texture2D noiseTex : register(t2);

SamplerState splr : register(s0);

cbuffer SkyboxTransCBuf : register(b2)
{
    // core
    int trState;
    int trMode;
    // parameters
    int fromIdx;
    int toIdx;
    float t;
    // radial transition
    float3 trCenter;
    float trRadius;
    float trSoftness;
    // noise
    float noiseScale;
    float noiseAmp;
};

#include "MathHelpers.hlsli"

float HexDist(float2 p)
{
    p = abs(p);
    return max(dot(p, normalize(float2(1.0, 1.73))), p.x);
}

float4 GetHexGrid(float2 uv)
{
    float2 r = float2(1.0, 1.73);
    float2 h = r * 0.5;
    
    float2 a = safe_mod(uv, r) - h;
    float2 b = safe_mod(uv - h, r) - h;
    
    float2 gv = dot(a, a) < dot(b, b) ? a : b;
    float2 id = uv - gv;
    
    return float4(gv.x, gv.y, id.x, id.y);
}

float4 main(float3 worldPos : Position) : SV_TARGET
{
    float3 dir = normalize(worldPos);

    float3 colA = texFrom.Sample(splr, dir).rgb;
    float3 colB = texTo.Sample(splr, dir).rgb;

    if (trState == 0)
    {
        return float4(colA, 1.0f);
    }

    float3 absPos = abs(worldPos);
    float maxAxis = max(absPos.x, max(absPos.y, absPos.z));

    float2 facePos;
    float2 faceCenter = float2(0.0f, 0.0f);

    if (absPos.x == maxAxis)
    {
        facePos = worldPos.yz;
    }
    else if (absPos.y == maxAxis)
    {
        facePos = worldPos.xz;
    }
    else
    {
        facePos = worldPos.xy;
    }

    const float dist = length(facePos - faceCenter);

    float3 result = colA;

    // ================================
    // MODE 0 - Normal radial blend
    // ================================
    if (trMode == 0)
    {
        float blend;
        if (trState == 1)
        {
            blend = 1.0f - smoothstep(trRadius - trSoftness, trRadius, dist);
        }
        else
        {
            blend = smoothstep(trRadius - trSoftness, trRadius, dist);
        }
        result = lerp(colA, colB, blend);
    }

    // ================================
    // MODE 1 - Burning (radial noise)
    // ================================
    else if (trMode == 1)
    {
        float ns = max(noiseScale, 0.0005f); // Noise frequency
        float band = max(trSoftness * 4.0f, trRadius * 1.0f); // Wide influence band
    
        // boundary distance: 0 at boundary, positive outside, negative inside
        float d = dist - trRadius;
    
        float mask = 1.0f - saturate(abs(d) / band);
        mask = mask * mask;
    
        // Noise amplitude
        float ampBase = trRadius * 0.20f + trSoftness * 6.0f;
        ampBase = min(ampBase, noiseAmp);
        ampBase = max(ampBase, trSoftness * 1.0f);
    
        float n = noiseTex.Sample(splr, facePos * ns).r;
        n = n * 2.0f - 1.0f;
    
        float noisyDist = dist + n * (ampBase * mask);
    
        float blend;
        if (trState == 1) // Expand
        {
            blend = 1.0f - smoothstep(trRadius - trSoftness, trRadius, noisyDist);
        }
        else // Recover
        {
            blend = smoothstep(trRadius - trSoftness, trRadius, noisyDist);
        }
        result = lerp(colA, colB, blend);
    
        // Ember edge
        float edgeWidth = max(trSoftness * 1.0f, 1.0f);
    
        float edge = 
        smoothstep(trRadius - edgeWidth, trRadius, noisyDist) -
        smoothstep(trRadius, trRadius + edgeWidth, noisyDist);
    
        edge = saturate(edge);
    
        float3 ember = float3(1.0f, 0.35f, 0.05f) * 2.0f;
        result += ember * edge;
    }

    // ================================
    // MODE 2 - Purifying (radial wave)
    // ================================
    else if (trMode == 2)
    {
        float2 dirVec = facePos - faceCenter;
        float ang = atan2(dirVec.y, dirVec.x);
    
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
        }
        else // Recover
        {
            blend = smoothstep(trRadius - trSoftness, trRadius, distWavy);
        }
        result = lerp(colA, colB, blend);
    
        // proportional to radius
        float rippleWidth = clamp(trRadius * 0.18f, 6.0f, 18.0f);
        rippleWidth = max(rippleWidth, trSoftness * 2.0f);
    
        if (distWavy < trRadius && distWavy > trRadius - rippleWidth)
        {
            float rippleMask = 1.0f - ((trRadius - distWavy) / rippleWidth);
        
            float surfaceWave = sin(facePos.x * 0.05f + facePos.y * 0.05f + trRadius * 0.1f);
            float2 sparkleUV = facePos * 0.02f + float2(trRadius * 0.01f, trRadius * 0.02f);
        
            float n = noiseTex.Sample(splr, sparkleUV).r;
            float sparkle = step(0.75f, n) * rippleMask;
        
            float3 waterColor = float3(0.0f, 0.7f, 1.0f);
            float3 sparkleColor = float3(1.0f, 1.0f, 1.0f);
        
            result += (waterColor * 0.6f * rippleMask) + (sparkleColor * sparkle);
        }
    }

    // ================================
    // MODE 3 - Digital Hex radial build
    // ================================
    else if (trMode == 3)
    {
        if(trState == 2)
        {
            float3 temp = colA;
            colA = colB;
            colB = temp;
        }
        
        float worldScale = 4.0f;
    
        float2 p = facePos * worldScale;
        float2 c = faceCenter * worldScale; // faceCenter��(0,0)�C����c�琥(0,0)
    
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
    
        result = lerp(colA, colB, insideSoft);
    
        // Glow border (base on threshold/R)
        float edgeWidth = 100.0f; // adjust the size of the border
        if (insideHard > 0.01f && (R - threshold) < edgeWidth)
        {
            float hDist = HexDist(hexInfo.xy);
            float border =
            smoothstep(0.40f, 0.45f, hDist) +
            smoothstep(0.48f, 0.50f, hDist) * 2.0f;
        
            float scanline = sin(facePos.x * 0.8f + R * 0.2f) * sin(facePos.y * 0.1f);
        
            float scanNoise = noiseTex.Sample(
            splr,
            float2(0.0f, facePos.y * 0.05f + R * 0.01f)
        ).r;
        
            float glitch = step(0.8f, scanNoise) * 2.0f;
        
            float3 cyan = float3(0.0f, 1.0f, 1.0f);
            float3 yellow = float3(1.0f, 0.8f, 0.2f);
            float3 emitColor = lerp(cyan, yellow, hexRand);
        
            float intensity = border * 1.5f + glitch + (scanline * 0.5f);
        
            // get brighter closer to the edge
            float fade = pow(saturate(1.0f - ((R - threshold) / edgeWidth)), 2.0f);
        
            result += emitColor * intensity * fade;
        }
    }

    return float4(result, 1.0f);
}