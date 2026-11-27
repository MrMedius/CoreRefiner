Texture2D texMain : register(t0);
Texture2D texNoise : register(t1);
SamplerState samp : register(s0);

cbuffer SkyGridParams : register(b1)
{
    float totalTime;    // Accumulated Time
    float rainbowSpeed; // Rainbow color change speed
    float rainbowScale; // Rainbow pattern zoom
    float globalAlpha;  // Global base opacity (0~2.5)

    float noiseScale;       // Noise UV scaling
    float noiseSpeed;       // Noise speed
    float noiseStrength;    // Noise intensity (0 = none, 1 = max)
    float transparencyFreq; // Periodic transparent frequency

    // Ripple data（ up to 4 ）
    // xy = UV center, z = radius, w = left life time (0~1)
    float4 rippleCenters[4];

    float rippleSoftness; 
    float rippleWidth;    
    float rippleIntensity;
    float fadeMultiplier;   // FadeOut 系数：1.0（fully visible）→ 0.0（fully hidden）
};

struct PSIn
{
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
    float4 pos : SV_Position;
};

// HSV → RGB 转换（H: 0~1, S: 0~1, V: 0~1）
float3 HSVtoRGB(float h, float s, float v)
{
    float3 rgb;
    float c = v * s;
    float hh = frac(h) * 6.0;
    float x = c * (1.0 - abs(fmod(hh, 2.0) - 1.0));
    float m = v - c;

    if (hh < 1.0)
        rgb = float3(c, x, 0);
    else if (hh < 2.0)
        rgb = float3(x, c, 0);
    else if (hh < 3.0)
        rgb = float3(0, c, x);
    else if (hh < 4.0)
        rgb = float3(0, x, c);
    else if (hh < 5.0)
        rgb = float3(x, 0, c);
    else
        rgb = float3(c, 0, x);

    return rgb + m;
}

float4 main(PSIn i) : SV_Target
{
    float2 uv = i.uv;

    // ---------------------------------------------------
    // 1. Main texture sampling ( mesh brightness )
    // ---------------------------------------------------
    float4 texColor = texMain.Sample(samp, uv);
    float gridBrightness = dot(texColor.rgb, float3(0.299, 0.587, 0.114));

    // ---------------------------------------------------
    // 2. Three layers and superimposed noise ( uneven flickering effect )
    // ---------------------------------------------------
    float2 nuv1 = uv * noiseScale;
    nuv1.x += totalTime * noiseSpeed * 0.7;
    nuv1.y += totalTime * noiseSpeed * 0.3;
    float n1 = texNoise.Sample(samp, nuv1).r;

    float2 nuv2 = uv * noiseScale * 1.8;
    nuv2.x -= totalTime * noiseSpeed * 0.5;
    nuv2.y += totalTime * noiseSpeed * 0.7;
    float n2 = texNoise.Sample(samp, nuv2).r;

    float2 nuv3 = uv * noiseScale * 3.2;
    nuv3.x += totalTime * noiseSpeed * 1.1;
    nuv3.y -= totalTime * noiseSpeed * 0.4;
    float n3 = texNoise.Sample(samp, nuv3).r;

    float combined = n1 * 0.4 + n2 * 0.35 + n3 * 0.25;
    float noiseMod = lerp(1.0, combined * combined + combined * 0.3, noiseStrength);
    noiseMod = max(noiseMod, 0.08);

    // ---------------------------------------------------
    // 3. Periodic transparent patterns ( overlay of horizontal / vertical / diagonal waves )
    // ---------------------------------------------------
    const float PI = 3.14159265;
    float p1 = sin(uv.x * transparencyFreq * PI + totalTime * 0.5) * 0.5 + 0.5;
    float p2 = sin(uv.y * transparencyFreq * 1.7 * PI + totalTime * 0.35) * 0.5 + 0.5;
    float p3 = sin((uv.x + uv.y) * transparencyFreq * 1.3 * PI + totalTime * 0.6) * 0.5 + 0.5;
    float p4 = sin((uv.x - uv.y * 0.7) * transparencyFreq * 0.9 * PI - totalTime * 0.25) * 0.5 + 0.5;

    float transPat = p1 * p2 * 0.6 + p3 * p4 * 0.4;
    transPat = lerp(0.15, 1.0, transPat);

    float patNoise = texNoise.Sample(samp, uv * 1.5 + float2(totalTime * 0.02, totalTime * 0.03)).r;
    transPat *= lerp(0.7, 1.0, patNoise);

    // ---------------------------------------------------
    // 4. Rainbow color animation ( HSV changes with UV + time )
    // ---------------------------------------------------
    float hue = frac(uv.x * rainbowScale * 0.2 + uv.y * rainbowScale * 0.1 + totalTime * rainbowSpeed * 0.1);
    float3 rainbow = HSVtoRGB(hue, 0.7, 1.0);

    // ---------------------------------------------------
    // 5.Ripple effect ( up to 4, with overlapping ring glows )
    // ---------------------------------------------------
    float rippleAccum = 0.0;
    for (int k = 0; k < 4; k++)
    {
        float4 rp = rippleCenters[k];
        float rLife = rp.w;
        if (rLife <= 0.0)
            continue;

        float2 rCenter = rp.xy;
        float rRadius = rp.z;

        float du = uv.x - rCenter.x;
        if (du > 0.5)
            du -= 1.0;
        if (du < -0.5)
            du += 1.0;
        float dv = uv.y - rCenter.y;
        float dist = sqrt(du * du + dv * dv);

        float ringDist = abs(dist - rRadius);
        float ring = 1.0 - smoothstep(0.0, rippleWidth + rippleSoftness, ringDist);
        float fadeFactor = rLife * rLife;

        float innerGlow = 0.0;
        if (dist < rRadius)
            innerGlow = exp(-(rRadius - dist) * 8.0) * 0.15;

        float outerGlow = exp(-ringDist * 12.0) * 0.25;

        rippleAccum += (ring + outerGlow + innerGlow) * fadeFactor * rippleIntensity;
    }
    rippleAccum = saturate(rippleAccum);

    // ---------------------------------------------------
    // 6. Final
    // ---------------------------------------------------
    float3 baseColor = gridBrightness * rainbow * noiseMod;
    float3 finalColor = baseColor + float3(rippleAccum, rippleAccum, rippleAccum);

    float finalAlpha = globalAlpha * texColor.a * transPat * noiseMod;
    finalAlpha = saturate(finalAlpha + rippleAccum * 0.6);
    finalAlpha *= lerp(0.4, 1.0, gridBrightness);

    return float4(finalColor * fadeMultiplier, finalAlpha * fadeMultiplier);
}