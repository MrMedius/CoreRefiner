#include "FireflyEffect.h"
#include "BindableCommon.h"
#include "DynamicConstant.h"
#include "DynamicVertexBuffer.h"
#include "Channels.h"
#include <DirectXMath.h>
#include <cassert>

#include "imgui/imgui.h"

namespace dx = DirectX;

FireflyEffect::FireflyEffect(Graphics& gfxIn, const Params& p)
    :
    gfx(gfxIn),
    params(p)
{
    InitParticles();

    BuildVertexBuffer();
    BuildStaticIndexBuffer();

    pTopology = Bind::Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto tcb = std::make_shared<Bind::TransformCbuf>(gfx);

    {
        Technique tech("Fireflies", Chan::main);
        Step step("lambertianTrans");

        // VS/PS
        auto pvs = Bind::VertexShader::Resolve(gfx, "Firefly_VS.cso");
        step.AddBindable(Bind::InputLayout::Resolve(gfx, cpuVB->GetLayout(), *pvs));
        step.AddBindable(std::move(pvs));
        step.AddBindable(Bind::PixelShader::Resolve(gfx, "Firefly_PS.cso"));

        // texture + sampler
        step.AddBindable(Bind::Texture::Resolve(gfx, "asset\\Images\\Environment\\noise.png", 0u));
        step.AddBindable(Bind::Sampler::Resolve(gfx, Bind::Sampler::Type::Anisotropic, false, 0u));

        // color cbuffer
        {
            auto buf = MakeColorBuffer(params.color);
            pColorCBuf = std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u);
            step.AddBindable(pColorCBuf);
        }

        // depth read only
        step.AddBindable(Bind::Stencil::Resolve(gfx, Bind::Stencil::Mode::DepthOnly));

        // particles are generally double-sided
        step.AddBindable(Bind::Rasterizer::Resolve(gfx, true));

        // blend -> Additive Mode
        step.AddBindable(Bind::Blender::Resolve(gfx, Bind::Blender::Mode::Additive));

        step.AddBindable(tcb);

        tech.AddStep(std::move(step));
        AddTechnique(std::move(tech));
    }
}

void FireflyEffect::SetParams(const Params& p)
{
    const bool countChanged = (p.count != params.count);
    params = p;

    InitParticles();

    if (countChanged)
    {
        BuildVertexBuffer();
        BuildStaticIndexBuffer();
    }

    // colors updating
    SetColor(params.color);
}

void FireflyEffect::SetColor(const DirectX::XMFLOAT4& c)
{
    params.color = c;
    if (pColorCBuf)
    {
        auto buf = MakeColorBuffer(params.color);
        pColorCBuf->SetBuffer(buf);
    }
}

void FireflyEffect::SetColor(float r, float g, float b, float a)
{
    SetColor(dx::XMFLOAT4{ r, g, b, a });
}


Dcb::Buffer FireflyEffect::MakeColorBuffer(const DirectX::XMFLOAT4& c)
{
    Dcb::RawLayout lay;
    lay.Add<Dcb::Float4>("particleColor");
    auto buf = Dcb::Buffer(std::move(lay));
    buf["particleColor"] = c;
    return buf;
}

void FireflyEffect::BuildVertexBuffer()
{
    Dvtx::VertexLayout layout;
    layout.Append(Dvtx::VertexLayout::Position3D);
    layout.Append(Dvtx::VertexLayout::Texture2D);
    layout.Append(Dvtx::VertexLayout::Float4Color);

    cpuVB.emplace(std::move(layout), (size_t)params.count * 4u);

    pDynVB = std::make_shared<Bind::DynamicVertexBuffer>(gfx, *cpuVB);
    pVertices = pDynVB;
}

void FireflyEffect::BuildStaticIndexBuffer()
{
    std::vector<unsigned short> indices;
    indices.reserve((size_t)params.count * 6u);

    for (int i = 0; i < params.count; i++)
    {
        const unsigned short base = (unsigned short)(i * 4);
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }

    const auto tag = "$firefly.ib." + std::to_string(params.count);
    pIndices = Bind::IndexBuffer::Resolve(gfx, tag, indices);
}

void FireflyEffect::SetPosition(dx::XMFLOAT3 pos) noexcept
{
    trans.SetPosition(pos.x, pos.y, pos.z);
}

dx::XMMATRIX FireflyEffect::GetTransformXM() const noexcept
{
    return trans.GetTransformXM();
}

void FireflyEffect::InitParticles()
{
    std::mt19937 rng(std::random_device{}());

    std::uniform_real_distribution<float> distX(params.rangeMin.x, params.rangeMax.x);
    std::uniform_real_distribution<float> distY(params.rangeMin.y, params.rangeMax.y);
    std::uniform_real_distribution<float> distZ(params.rangeMin.z, params.rangeMax.z);

    std::uniform_real_distribution<float> distAmp(params.ampMin, params.ampMax);
    std::uniform_real_distribution<float> distMove(params.moveSpeedMin, params.moveSpeedMax);
    std::uniform_real_distribution<float> distPhase(0.0f, dx::XM_2PI);

    std::uniform_real_distribution<float> distBlink(params.blinkSpeedMin, params.blinkSpeedMax);
    std::uniform_real_distribution<float> distSize(params.sizeMin, params.sizeMax);

    particles.clear();
    particles.resize(params.count);

    for (auto& p : particles)
    {
        p.basePos = { distX(rng), distY(rng), distZ(rng) };
        p.curPos = p.basePos;

        p.amp = { distAmp(rng), distAmp(rng), distAmp(rng) };
        p.speed = { distMove(rng), distMove(rng), distMove(rng) };
        p.phase = { distPhase(rng), distPhase(rng), distPhase(rng) };

        p.size = distSize(rng);

        p.blinkPhase = distPhase(rng);
        p.blinkSpeed = distBlink(rng);

        p.fade = 1.0f;
    }
}

void FireflyEffect::Update(float dt)
{
    timeSec += dt;

    for (auto& p : particles)
    {
        p.curPos.x = p.basePos.x + sinf(timeSec * p.speed.x + p.phase.x) * p.amp.x;
        p.curPos.y = p.basePos.y + sinf(timeSec * p.speed.y + p.phase.y) * p.amp.y;
        p.curPos.z = p.basePos.z + sinf(timeSec * p.speed.z + p.phase.z) * p.amp.z;

        // blink
        float t = 1.0f;
        if (p.blinkSpeed > 0.0f)
        {
            // 0..1
            const float s = sinf(timeSec * p.blinkSpeed + p.blinkPhase);
            t = (s + 1.0f) * 0.5f;
        }
        // Curves: Make the transition from dark to light softer/more like a firefly.
        t = std::clamp(t, 0.0f, 1.0f);
        t = powf(t, std::max(0.1f, params.blinkPow));
        // Lower limit: Guaranteed not to disappear
        const float minA = std::clamp(params.blinkMin, 0.0f, 1.0f);
        p.fade = minA + (1.0f - minA) * t;
    }

    UpdateVerticesBillboard();
}

void FireflyEffect::UpdateVerticesBillboard()
{
    assert(cpuVB.has_value());
    assert(pDynVB);

    dx::XMFLOAT4X4 view;
    dx::XMStoreFloat4x4(&view, gfx.GetCamera());

    dx::XMFLOAT3 right = { view._11, view._21, view._31 };
    dx::XMFLOAT3 up = { view._12, view._22, view._32 };

    using Type = Dvtx::VertexLayout::ElementType;
    auto& vb = *cpuVB;

    for (int i = 0; i < params.count; ++i)
    {
        const auto& p = particles[i];
        const float half = p.size * 0.5f;

        dx::XMVECTOR C = dx::XMLoadFloat3(&p.curPos);
        dx::XMVECTOR R = dx::XMLoadFloat3(&right) * half;
        dx::XMVECTOR U = dx::XMLoadFloat3(&up) * half;

        dx::XMVECTOR p0 = C - R + U;
        dx::XMVECTOR p1 = C + R + U;
        dx::XMVECTOR p2 = C - R - U;
        dx::XMVECTOR p3 = C + R - U;

        const int base = i * 4;

        dx::XMStoreFloat3(&vb[base + 0].Attr<Type::Position3D>(), p0);
        vb[base + 0].Attr<Type::Texture2D>() = { 0.0f, 0.0f };
        vb[base + 0].Attr<Type::Float4Color>() = { p.fade, 0, 0, 0 };

        dx::XMStoreFloat3(&vb[base + 1].Attr<Type::Position3D>(), p1);
        vb[base + 1].Attr<Type::Texture2D>() = { 1.0f, 0.0f };
        vb[base + 1].Attr<Type::Float4Color>() = { p.fade, 0, 0, 0 };

        dx::XMStoreFloat3(&vb[base + 2].Attr<Type::Position3D>(), p2);
        vb[base + 2].Attr<Type::Texture2D>() = { 0.0f, 1.0f };
        vb[base + 2].Attr<Type::Float4Color>() = { p.fade, 0, 0, 0 };

        dx::XMStoreFloat3(&vb[base + 3].Attr<Type::Position3D>(), p3);
        vb[base + 3].Attr<Type::Texture2D>() = { 1.0f, 1.0f };
        vb[base + 3].Attr<Type::Float4Color>() = { p.fade, 0, 0, 0 };
    }

    pDynVB->Update(gfx, vb);
}

void FireflyEffect::SpawnWindow()
{
    if (!ImGui::Begin("Firefly Effect"))
    {
        ImGui::End();
        return;
    }

    // color
    {
        float col[4] = { params.color.x, params.color.y, params.color.z, params.color.w };
        if (ImGui::ColorEdit4("Color", col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
        {
            SetColor(col[0], col[1], col[2], col[3]);
        }
    }

    ImGui::Separator();

    // change count and rebuild VB/IB
    {
        static int   uiCount = 0;
        static bool  init = false;
        if (!init)
        {
            uiCount = params.count;
            init = true;
        }
        ImGui::TextUnformatted("Particles");
        ImGui::PushItemWidth(200);
        ImGui::InputInt("Count", &uiCount);
        uiCount = (uiCount < 1) ? 1 : uiCount;
        uiCount = (uiCount > 20000) ? 20000 : uiCount;
        if (ImGui::Button("Apply Count (Rebuild)"))
        {
            auto p = params;
            p.count = uiCount;
            SetParams(p);
        }
        ImGui::PopItemWidth();
    }

    ImGui::Separator();

    // size
    {
        float sMin = params.sizeMin;
        float sMax = params.sizeMax;
        if (ImGui::DragFloatRange2("Size Min/Max", &sMin, &sMax, 0.1f, 0.01f, 200.0f, "Min: %.2f", "Max: %.2f"))
        {
            if (sMin < 0.01f) sMin = 0.01f;
            if (sMax < sMin)  sMax = sMin;

            auto p = params;
            p.sizeMin = sMin;
            p.sizeMax = sMax;
            SetParams(p);
        }
    }

    // amp
    {
        float aMin = params.ampMin;
        float aMax = params.ampMax;
        if (ImGui::DragFloatRange2("Amp Min/Max", &aMin, &aMax, 0.1f, 0.0f, 500.0f, "Min: %.2f", "Max: %.2f"))
        {
            if (aMin < 0.0f) aMin = 0.0f;
            if (aMax < aMin) aMax = aMin;

            auto p = params;
            p.ampMin = aMin;
            p.ampMax = aMax;
            SetParams(p);
        }
    }

    // move speed
    {
        float mMin = params.moveSpeedMin;
        float mMax = params.moveSpeedMax;
        if (ImGui::DragFloatRange2("MoveSpeed Min/Max", &mMin, &mMax, 0.01f, 0.0f, 20.0f, "Min: %.2f", "Max: %.2f"))
        {
            if (mMin < 0.0f) mMin = 0.0f;
            if (mMax < mMin) mMax = mMin;

            auto p = params;
            p.moveSpeedMin = mMin;
            p.moveSpeedMax = mMax;
            SetParams(p);
        }
    }

    // blink speed
    {
        float bMin = params.blinkSpeedMin;
        float bMax = params.blinkSpeedMax;
        if (ImGui::DragFloatRange2("BlinkSpeed Min/Max", &bMin, &bMax, 0.01f, 0.0f, 50.0f, "Min: %.2f", "Max: %.2f"))
        {
            if (bMin < 0.0f) bMin = 0.0f;
            if (bMax < bMin) bMax = bMin;

            auto p = params;
            p.blinkSpeedMin = bMin;
            p.blinkSpeedMax = bMax;
            SetParams(p);
        }
    }

    // blink min
    {
        float minA = params.blinkMin;
        if (ImGui::SliderFloat("Blink Min", &minA, 0.0f, 1.0f, "%.2f"))
        {
            auto pp = params;
            pp.blinkMin = minA;
            SetParams(pp);
        }
    }

    // blink pow
    {
        float pw = params.blinkPow;
        if (ImGui::SliderFloat("Blink Pow", &pw, 0.1f, 6.0f, "%.2f"))
        {
            auto pp = params;
            pp.blinkPow = pw;
            SetParams(pp);
        }
    }

    ImGui::Separator();

    // range
    {
        float rMin[3] = { params.rangeMin.x, params.rangeMin.y, params.rangeMin.z };
        float rMax[3] = { params.rangeMax.x, params.rangeMax.y, params.rangeMax.z };

        bool changed = false;
        changed |= ImGui::DragFloat3("Range Min", rMin, 1.0f);
        changed |= ImGui::DragFloat3("Range Max", rMax, 1.0f);

        if (changed)
        {
            // ensure min <= max
            for (int k = 0; k < 3; ++k)
            {
                if (rMin[k] > rMax[k]) std::swap(rMin[k], rMax[k]);
            }

            auto p = params;
            p.rangeMin = { rMin[0], rMin[1], rMin[2] };
            p.rangeMax = { rMax[0], rMax[1], rMax[2] };
            SetParams(p);
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Reseed (Reinit Particles)"))
    {
        // reset random distribution
        auto p = params;
        SetParams(p);
    }

    ImGui::End();
}