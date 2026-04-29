#include "SkyGridCylinder.h"
#include "BindableCommon.h"
#include "Channels.h"
#include "DynamicVertex.h"
#include <cmath>
#include <algorithm>

namespace dx = DirectX;
using namespace Bind;

SkyGridCylinder::SkyGridCylinder(Graphics& gfxIn, const Params& p)
    :
    m_params(p),
    gfx(gfxIn)
{
    m_rng.seed(std::random_device{}());
    m_rippleInterval = m_params.rippleIntervalMin;

    Dvtx::VertexLayout layout;
    layout.Append(Dvtx::VertexLayout::Position3D);
    layout.Append(Dvtx::VertexLayout::Texture2D);
    layout.Append(Dvtx::VertexLayout::Normal);

    const int   seg = m_params.segments;
    const float r = m_params.radius;
    const float halfH = m_params.height * 0.5f;
    const float uTile = 4.0f;
    const float vTile = 3.0f;

    Dvtx::VertexBuffer cpuVB(layout, (size_t)(seg + 1) * 2);

    for (int i = 0; i <= seg; ++i)
    {
        float angle = (float)i / (float)seg * dx::XM_2PI;
        float x = cosf(angle) * r;
        float z = sinf(angle) * r;
        float nx = -cosf(angle); // Inner normal (towards the center)
        float nz = -sinf(angle);
        float u = (float)i / (float)seg * uTile;

        int base = i * 2;

        // top
        cpuVB[base + 0].Attr<Dvtx::VertexLayout::Position3D>() = dx::XMFLOAT3{ x,  halfH, z };
        cpuVB[base + 0].Attr<Dvtx::VertexLayout::Texture2D>() = dx::XMFLOAT2{ u, 0.0f };
        cpuVB[base + 0].Attr<Dvtx::VertexLayout::Normal>() = dx::XMFLOAT3{ nx, 0.0f, nz };

        // bottom
        cpuVB[base + 1].Attr<Dvtx::VertexLayout::Position3D>() = dx::XMFLOAT3{ x, -halfH, z };
        cpuVB[base + 1].Attr<Dvtx::VertexLayout::Texture2D>() = dx::XMFLOAT2{ u, vTile };
        cpuVB[base + 1].Attr<Dvtx::VertexLayout::Normal>() = dx::XMFLOAT3{ nx, 0.0f, nz };
    }

    std::vector<unsigned short> indices;
    indices.reserve((size_t)seg * 6);
    for (int i = 0; i < seg; ++i)
    {
        unsigned short tl = (unsigned short)(i * 2);
        unsigned short bl = (unsigned short)(i * 2 + 1);
        unsigned short tr = (unsigned short)((i + 1) * 2);
        unsigned short br = (unsigned short)((i + 1) * 2 + 1);
        indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
        indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
    }

    // Drawable -> pVertices / pIndices / pTopology
    const std::string geoTag = "$skygrid.r" + std::to_string((int)r) + ".s" + std::to_string(seg);
    pVertices = VertexBuffer::Resolve(gfx, geoTag, cpuVB);
    pIndices = IndexBuffer::Resolve(gfx, geoTag, indices);
    pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // PixelConstantBuffer
    m_pParamCBuf = std::make_shared<PixelConstantBuffer<SkyGridParams>>(gfx, m_skyParams, 1u);
    auto tcb = std::make_shared<TransformCbuf>(gfx);

    {
        Technique tech("SkyGrid", Chan::main);
        Step step("lambertianTrans");

        auto pvs = VertexShader::Resolve(gfx, "SkyGrid_VS.cso");
        step.AddBindable(InputLayout::Resolve(gfx, cpuVB.GetLayout(), *pvs));
        step.AddBindable(std::move(pvs));

        step.AddBindable(PixelShader::Resolve(gfx, "SkyGrid_PS.cso"));

        // skygrid -> slot 0
        // noise -> slot 1
        step.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\skygrid.png", 0u));
        step.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\noise.png", 1u));
        step.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Anisotropic, true, 0u));

        step.AddBindable(m_pParamCBuf);

        step.AddBindable(Blender::Resolve(gfx, Blender::Mode::Additive));
        step.AddBindable(Rasterizer::Resolve(gfx, true));
        step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthOnly));

        step.AddBindable(tcb);

        tech.AddStep(std::move(step));
        AddTechnique(std::move(tech));
    }
}

void SkyGridCylinder::Update(float dt)
{
    if (m_fadeFinished) return;

    m_totalTime += dt;

    // Rotate
    m_trans.RotateRad(0.0f, m_params.rotationSpeedRad, 0.0f);

    // FadeOut
    if (m_fadingOut)
    {
        m_fadeProgress += dt;
        float t = std::clamp(m_fadeProgress / m_fadeDuration, 0.0f, 1.0f);
        float eased = 1.0f - (1.0f - t) * (1.0f - t);
        m_skyParams.fadeMultiplier = 1.0f - eased;   // 1.0 → 0.0
        if (t >= 1.0f)
        {
            m_fadeFinished = true;
            m_skyParams.fadeMultiplier = 0.0f;
        }
    }
    else
    {
        // Pulse
        float s = sinf(m_totalTime * m_params.pulseSpeed);
        float n = (s + 1.0f) * 0.5f;
        m_skyParams.globalAlpha = m_params.pulseAlphaMin + n * (m_params.pulseAlphaMax - m_params.pulseAlphaMin);
    }

    // pass time to PS
    m_skyParams.totalTime = m_totalTime;

    // ripples update
    UpdateRipples(dt);

    // random ripple
    if (!m_fadingOut)
    {
        m_rippleTimer += dt;
        if (m_rippleTimer >= m_rippleInterval)
        {
            m_rippleTimer = 0.0f;
            SpawnRandomRipple();
            std::uniform_real_distribution<float> d(m_params.rippleIntervalMin, m_params.rippleIntervalMax);
            m_rippleInterval = d(m_rng);
        }
    }

    // update ripple param to cbuf struct
    for (int i = 0; i < MAX_RIPPLES; i++)
    {
        if (m_ripples[i].active)
        {
            m_skyParams.rippleCenters[i] = dx::XMFLOAT4{
                m_ripples[i].centerU,
                m_ripples[i].centerV,
                m_ripples[i].currentRadius,
                m_ripples[i].life
            };
        }
        else
        {
            m_skyParams.rippleCenters[i] = dx::XMFLOAT4{ 0, 0, 0, 0 };
        }
    }

    m_pParamCBuf->Update(gfx, m_skyParams);
}

dx::XMMATRIX SkyGridCylinder::GetTransformXM() const noexcept
{
    return m_trans.GetTransformXM();
}

void SkyGridCylinder::StartFadeOut(float duration)
{
    if (m_fadingOut || m_fadeFinished) return;
    m_fadingOut = true;
    m_fadeProgress = 0.0f;
    m_fadeDuration = duration;
}

void SkyGridCylinder::SetParams(const SkyGridParams& p)
{
    m_skyParams = p;
}


void SkyGridCylinder::UpdateRipples(float dt)
{
    for (auto& r : m_ripples)
    {
        if (!r.active) continue;
        r.currentRadius += r.speed * dt;
        r.life -= dt / 2.8f;
        if (r.life <= 0.0f || r.currentRadius >= r.maxRadius)
        {
            r.active = false;
            r.life = 0.0f;
        }
    }
}

void SkyGridCylinder::SpawnRandomRipple()
{
    int slot = -1;
    for (int i = 0; i < MAX_RIPPLES; i++)
        if (!m_ripples[i].active) { slot = i; break; }
    if (slot < 0) return;

    std::uniform_real_distribution<float> dU(0.0f, 1.0f);
    std::uniform_real_distribution<float> dV(0.05f, 1.95f);
    std::uniform_real_distribution<float> dSpd(0.15f, 0.3f);
    std::uniform_real_distribution<float> dRad(0.5f, 1.0f);

    const float MIN_DIST = 0.4f;
    for (int attempt = 0; attempt < 5; attempt++)
    {
        float cu = dU(m_rng);
        float cv = dV(m_rng);

        bool tooClose = false;
        for (int i = 0; i < MAX_RIPPLES; i++)
        {
            if (!m_ripples[i].active) continue;
            float du = fabsf(cu - m_ripples[i].centerU);
            if (du > 0.5f) du = 1.0f - du;
            float dv = fabsf(cv - m_ripples[i].centerV);
            if (sqrtf(du * du + dv * dv) < MIN_DIST) { tooClose = true; break; }
        }
        if (!tooClose)
        {
            m_ripples[slot].centerU = cu;
            m_ripples[slot].centerV = cv;
            m_ripples[slot].currentRadius = 0.0f;
            m_ripples[slot].maxRadius = dRad(m_rng);
            m_ripples[slot].life = 1.0f;
            m_ripples[slot].speed = dSpd(m_rng);
            m_ripples[slot].active = true;
            break;
        }
    }
}

void SkyGridCylinder::Reset()
{
    // Ripples
    m_rippleTimer = 0.0f;
    m_rippleInterval = m_params.rippleIntervalMin;
    for (int i = 0; i < MAX_RIPPLES; i++)
    {
        m_ripples[i].active = false;
        m_ripples[i].centerU = 0.0f;
        m_ripples[i].centerV = 0.0f;
        m_ripples[i].currentRadius = 0.0f;
        m_ripples[i].maxRadius = 0.0f;
        m_ripples[i].life = 0.0f;
        m_ripples[i].speed = 0.0f;
    }

    // Time
    m_totalTime = 0.0f;

    // Fade Out
    m_fadingOut = false;
    m_fadeFinished = false;
    m_fadeProgress = 0.0f;
    m_fadeDuration = 2.0f;

    // Sky Params
    m_skyParams = SkyGridParams{};
    for (int i = 0; i < 4; i++)
    {
        m_skyParams.rippleCenters[i] = dx::XMFLOAT4{ 0, 0, 0, 0 };
    }

    // Transformation
    m_trans.SetRotationRad(0.0f, 0.0f, 0.0f);

    // Constant Buffer
    m_pParamCBuf->Update(gfx, m_skyParams);
}