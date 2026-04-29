#pragma once
#include "Drawable.h"
#include "Transformation.h"
#include "ConstantBuffers.h"
#include "DynamicVertex.h"
#include <vector>
#include <random>
#include <DirectXMath.h>

class SkyGridCylinder : public Drawable
{
public:
    struct SkyGridParams
    {
        float totalTime = 0.0f;
        float rainbowSpeed = 1.5f;
        float rainbowScale = 3.0f;
        float globalAlpha = 10.0f;

        float noiseScale = 8.0f;
        float noiseSpeed = 0.05f;
        float noiseStrength = 0.6f;
        float transparencyFreq = 8.0f;

        DirectX::XMFLOAT4 rippleCenters[4] = {};

        float rippleSoftness = 0.025f;
        float rippleWidth = 0.012f;
        float rippleIntensity = 0.6f;
        float fadeMultiplier = 1.0f;
    };

    struct Params
    {
        float radius = 100.0f;
        float height = 300.0f;
        int   segments = 64;

        // Pulse
        float pulseSpeed = 0.3f;
        float pulseAlphaMin = 0.5f;
        float pulseAlphaMax = 2.5f;

        // Rotate
        float rotationSpeedRad = -0.0003f;

        // Ripple Interval
        float rippleIntervalMin = 1.5f;
        float rippleIntervalMax = 3.5f;
    };

public:
    SkyGridCylinder(Graphics& gfxIn, const Params& p = {});
    void Update(float dt);
    DirectX::XMMATRIX GetTransformXM() const noexcept override;

    // Fade Out Control
    void StartFadeOut(float duration = 2.0f);
    bool IsFadeOutFinished() const { return m_fadeFinished; }

    // Set Params
    void SetParams(const SkyGridParams& p);
    const SkyGridParams& GetSkyParams() const { return m_skyParams; }

    void Reset();

private:
    // CPU RippleData
    struct RippleData
    {
        float centerU = 0.0f;
        float centerV = 0.0f;
        float currentRadius = 0.0f;
        float maxRadius = 0.0f;
        float life = 0.0f;   // 1 -> 0
        float speed = 0.0f;
        bool  active = false;
    };

    void UpdateRipples(float dt);
    void SpawnRandomRipple();

    static constexpr int MAX_RIPPLES = 4;

private:
    Params           m_params;
    SkyGridParams    m_skyParams;
    Transformation   m_trans;

    // Ripples
    RippleData m_ripples[MAX_RIPPLES];
    float      m_rippleTimer = 0.0f;
    float      m_rippleInterval = 2.0f;

    // Time
    float m_totalTime = 0.0f;

    // Fade Out
    bool  m_fadingOut = false;
    bool  m_fadeFinished = false;
    float m_fadeProgress = 0.0f;
    float m_fadeDuration = 2.0f;

    std::shared_ptr<Bind::PixelConstantBuffer<SkyGridParams>> m_pParamCBuf;

    // Random Engine
    std::mt19937 m_rng;

    Graphics& gfx;
};