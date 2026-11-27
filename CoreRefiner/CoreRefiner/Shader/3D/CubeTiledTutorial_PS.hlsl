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

Texture2D noiseTex : register(t1);
SamplerState splr : register(s0);

float4 main(
    float3 viewFragPos : Position,
    float3 viewNormal : Normal,
    float2 tc : Texcoord,
    float4 spos : ShadowPosition,
    float3 worldPos : WorldPos) : SV_Target
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

    // Basic Color
    float3 baseColor = float3(0.4, 0.4, 0.4);
    
    float3 lit0 = saturate((diffuse + ambient) * baseColor + specular);
    return float4(lit0, 1.0);
}