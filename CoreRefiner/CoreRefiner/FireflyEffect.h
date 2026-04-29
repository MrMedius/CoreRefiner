#pragma once
#include "Drawable.h"
#include "Transformation.h"
#include "DynamicVertex.h"
#include "ConstantBuffersEx.h"
#include <random>
#include <vector>
#include <optional>
#include <memory>

class Camera;

namespace Bind
{
    class DynamicVertexBuffer;
}

class FireflyEffect : public Drawable
{
public:
    struct Params
    {
        int count = 500;

        DirectX::XMFLOAT3 rangeMin = { -500.f, -200.f, -500.f };
        DirectX::XMFLOAT3 rangeMax = { 500.f,  200.f,  500.f };

        float sizeMin = 1.0f;
        float sizeMax = 3.0f;

        // wiggle
        float ampMin = 10.0f;
        float ampMax = 20.0f;
        float moveSpeedMin = 0.5f;  // rad / s
        float moveSpeedMax = 1.0f;

        // blinking
        float blinkSpeedMin = 1.0f; // rad/s
        float blinkSpeedMax = 3.0f;
        float blinkMin = 0.25f;
        float blinkPow = 1.6f;

        DirectX::XMFLOAT4 color = { 0.1f, 0.1f, 0.1f, 0.5f };
    };

public:
    FireflyEffect(Graphics& gfx, const Params& p = {});

    void Update(float dt);

    // setters to change params when running
    void SetParams(const Params& p);
    void SetColor(const DirectX::XMFLOAT4& c);
    void SetColor(float r, float g, float b, float a);
    void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
    DirectX::XMMATRIX GetTransformXM() const noexcept override;

    void SpawnWindow();

private:
    struct Particle
    {
        DirectX::XMFLOAT3 basePos;
        DirectX::XMFLOAT3 curPos;
        DirectX::XMFLOAT3 amp;
        DirectX::XMFLOAT3 speed;
        DirectX::XMFLOAT3 phase;
        float size = 1.0f;

        float blinkPhase = 0.0f;
        float blinkSpeed = 1.0f;
        float fade = 1.0f;
    };

private:
    void InitParticles();
    void BuildStaticIndexBuffer();
    void BuildVertexBuffer();
    void UpdateVerticesBillboard();

    static Dcb::Buffer MakeColorBuffer(const DirectX::XMFLOAT4& c);

private:
    Graphics& gfx;

    Params params;
    std::vector<Particle> particles;

    std::optional<Dvtx::VertexBuffer> cpuVB;
    std::shared_ptr<Bind::DynamicVertexBuffer> pDynVB;
    std::shared_ptr<Bind::CachingPixelConstantBufferEX> pColorCBuf;

    float timeSec = 0.0f;
    Transformation trans;
};
