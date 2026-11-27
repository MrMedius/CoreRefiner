#include "ShaderOps.hlsli"
#include "LightVectorData.hlsli"
#include "PointLight.hlsli"
#include "PShadow.hlsli"

cbuffer ObjectCBuf : register(b1)
{
    float3 specularColor;
    float specularWeight;
    float specularGloss;
};

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
SamplerState splr : register(s0);

// HSV -> RGB
float3 HSVtoRGB(float h, float s, float v)
{
    float c = v * s;
    float hPrime = fmod(h * 6.0, 6.0);
    float x = c * (1.0 - abs(fmod(hPrime, 2.0) - 1.0));
    float m = v - c;
    float3 rgb;
    if (hPrime < 1.0)
        rgb = float3(c, x, 0.0);
    else if (hPrime < 2.0)
        rgb = float3(x, c, 0.0);
    else if (hPrime < 3.0)
        rgb = float3(0.0, c, x);
    else if (hPrime < 4.0)
        rgb = float3(0.0, x, c);
    else if (hPrime < 5.0)
        rgb = float3(x, 0.0, c);
    else
        rgb = float3(c, 0.0, x);
    return rgb + m;
}

float4 main(
    float3 viewFragPos : Position,
    float3 viewNormal : Normal,
    float2 tc : Texcoord,
    float4 spos : ShadowPosition,
    float3 worldPos : WorldPos) : SV_Target
{
    float3 diff = worldPos - float3(0.0, 0.0, 0.0); // Fixed origin of the collapse center
    float dist = length(float2(diff.x, diff.z));
    float angle = atan2(diff.z, diff.x);

    // Vortex Offset
    float vortexOffset = sin(angle * 3.0 + totalTime * 4.0) * vortexStrength;
    float distWithVortex = dist + vortexOffset;

    // Noise disturbance
    float2 noiseUV = tc * 2.0 + float2(totalTime * 0.3, totalTime * 0.2);
    float noiseVal = texNoise.Sample(splr, noiseUV).r;
    float noiseOffset = (noiseVal - 0.5) * trSoftness * 0.8;

    float finalDist = distWithVortex + noiseOffset;

    // Ripple
    float ripple = sin(dist * rippleCount * 1.8 - totalTime * 6.0) * 0.5 + 0.5;
    float borderDist = abs(dist - trRadius);
    float rippleInfluence = 1.0 - smoothstep(0.0, trSoftness * 4.0, borderDist);
    float outsideMask = step(trRadius, dist); // dist >= trRadius ??1ÅCì¥ì‡?0
    finalDist += ripple * rippleInfluence * trSoftness * 0.3 * outsideMask;

    // Collapse Mask
    float collapseMask = smoothstep(trRadius - trSoftness, trRadius + trSoftness * 0.5, finalDist);
    float edgeProximity = 1.0 - smoothstep(0.0, trSoftness * 4.0, borderDist);
    if (collapseMask < 0.01 && edgeProximity < 0.01)
        discard;
    
    
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
    
    // Basic Color
    float3 base = float3(0.4, 0.4, 0.4);
    float3 lit0 = saturate((diffuse + ambient) * base + specular);
    float4 baseColor = float4(lit0, 1.0);
    
    
    // Rainbow Border
    float borderWidth = trSoftness * 0.5;
    float borderGlow = pow(1.0 - smoothstep(0.0, borderWidth, borderDist), 1.5);

    float hue = frac(
        angle / (3.14159 * 2.0)
        + dist * 0.005
        + totalTime * 0.8
        + noiseVal * 0.15
    );
    float3 rainbowColor = HSVtoRGB(hue, 0.9, 1.0);

    float ripplePulse = sin(dist * rippleCount * 1.8 - totalTime * 6.0) * 0.5 + 0.5;
    float rippleGlow = ripplePulse * rippleInfluence * 0.6;

    float innerRingDist = abs(dist - (trRadius - trSoftness * 0.3));
    float outerRingDist = abs(dist - (trRadius + trSoftness * 0.3));
    float rainbowWidth = trSoftness * 3.0;
    float innerRing = exp(-innerRingDist * innerRingDist / rainbowWidth);
    float outerRing = exp(-outerRingDist * outerRingDist / (rainbowWidth * 1.5));

    float hue2 = frac(-angle / (3.14159 * 2.0) + dist * 0.008 + totalTime * 1.2);
    float3 rainbowColor2 = HSVtoRGB(hue2, 1.0, 1.0);

    float3 totalRainbow = (rainbowColor * (borderGlow + outerRing * collapseMask)
                         + rainbowColor * rippleGlow)
                         * rainbowIntensity;

    // Final
    float3 finalColor = baseColor.rgb * collapseMask + totalRainbow;
    finalColor += borderGlow * rainbowIntensity * 0.3; // bloom

    float glowAlpha = saturate(borderGlow * rainbowIntensity * 0.8 + rippleGlow * rainbowIntensity * 0.3);
    float finalAlpha = max(collapseMask * baseColor.a, glowAlpha);

    if (finalAlpha < 0.005)
        discard;

    return float4(finalColor, finalAlpha);
}