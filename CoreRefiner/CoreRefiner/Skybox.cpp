#include "Skybox.h"
#include "BindableCommon.h"
#include "Cube.h"
#include "SkyboxTransformCbuf.h"
#include "SkyboxTransitionCbuf.h"
#include "Channels.h"

Skybox::Skybox(Graphics& gfx, const std::vector<std::string>& folders)
{
    using namespace Bind;

    // geometry
    auto model = Cube::Make();
    model.Transform(DirectX::XMMatrixScaling(100.0f, 100.0f, 100.0f));
    const auto geometryTag = "$skybox_cube";
    auto layout = model.vertices.GetLayout();

    pVertices = VertexBuffer::Resolve(gfx, geometryTag, std::move(model.vertices));
    pIndices = IndexBuffer::Resolve(gfx, geometryTag, std::move(model.indices));
    pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // cbufs
    auto tcb = std::make_shared<SkyboxTransformCbuf>(gfx);
    // SkyboxTransitionCbuf -> t0/t1 + blend cbuffer(b2)
    auto stcb = std::make_shared<SkyboxTransitionCbuf>(gfx, folders, 0u, 1u, 2u);

    Technique sky("Skybox", Chan::main);
    {
        Step step("skybox");

        auto pvs = VertexShader::Resolve(gfx, "Skybox_VS.cso");
        step.AddBindable(InputLayout::Resolve(gfx, layout, *pvs));
        step.AddBindable(std::move(pvs));

        step.AddBindable(PixelShader::Resolve(gfx, "Skybox_PS.cso"));

        step.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Bilinear));
        step.AddBindable(Rasterizer::Resolve(gfx, true));
        step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));

        step.AddBindable(tcb);
        step.AddBindable(stcb);
        step.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\block_noise_cloud.png", 2u));

        sky.AddStep(std::move(step));
    }
    AddTechnique(std::move(sky));

    // init
    state = State::None;
    mode = 0;
    fromIdx = 0;
    toIdx = 0;
    t = 0.0f;
    radius = 0.0f;
    center = { 0.0f, 0.0f, 0.0f };
}

DirectX::XMMATRIX Skybox::GetTransformXM() const noexcept
{
    return DirectX::XMMatrixIdentity();
}

void Skybox::Update(float dt)
{
    switch (state)
    {
    case State::None:
        break;

    case State::Expand:
        t += dt * interval;
        t = (t > 1.0f) ? 1.0f : t;

        radius = t * maxRadius;

        if (t >= 1.0f)
        {
            // start Recover automatically after Expend
            StartRecover();
        }
        break;

    case State::Recover:
        t -= dt * interval;
        t = (t < 0.0f) ? 0.0f : t;

        radius = t * maxRadius;

        if (t <= 0.0f)
        {
            FinishRecover();
            fromIdx = 0;
            toIdx = fromIdx;
        }
        break;
    }
}

void Skybox::StartExpand(int newMode)
{
    fromIdx = toIdx;
    toIdx = newMode;
    // core
    state = State::Expand;
    mode = newMode;
    // animation control
    t = 0.0f;
    // shader params
    radius = 0.0f;
}

void Skybox::StartRecover()
{
    fromIdx = toIdx;
    toIdx = 0;
    // core
    state = State::Recover;
    // animation control
    t = 1.0f;
    // shader params
    radius = maxRadius;
}

void Skybox::FinishRecover()
{
    state = State::None;
    mode = 0;
}

void Skybox::ResetMode()
{
    state = State::None;
    mode = 0;
    fromIdx = 0;
    toIdx = 0;
    t = 0.0f;
    radius = 0.0f;
    center = { 0.0f, 0.0f, 0.0f };
}

SkyboxTransitionTag::value_type Skybox::Provide(SkyboxTransitionTag) const noexcept
{
    return {
    // core
    static_cast<int>(state),
    mode,
    // parameters
    fromIdx,
    toIdx,
    t,
    // radial transition
    center,  
    radius,
    softness,
    // noise
    noiseScale,
    noiseAmp
    };
}
