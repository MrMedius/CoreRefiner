#include "CubeTiledTutorial.h"
#include "IndexedTriangleList.h"
#include "DynamicVertex.h"
#include "BindableCommon.h"
#include "ConstantBuffersEx.h"
#include "TutorialTransitionCbuf.h"
#include "Channels.h"
#include <algorithm>
#include <cmath>

using dvL = Dvtx::VertexLayout;

CubeTiledTutorial::CubeTiledTutorial(Graphics& gfx, XMFLOAT3 size, XMFLOAT2 numTiled)
{
    using namespace Bind;

    m_size = size;
    m_numTiled = numTiled;

    FT = TutorialTransitionTag::value_type{};
    FT.trSoftness = 40.0f;
    FT.numTiles = numTiled;

    const auto geometryTag = "$cubeTut." + std::to_string(numTiled.x) + "x" + std::to_string(numTiled.y);

    Dvtx::VertexLayout layout;
    layout.Append(dvL::Position3D).Append(dvL::Normal).Append(dvL::Texture2D);
    Dvtx::VertexBuffer vertices{ std::move(layout) };
    vertices.Resize(24u);
    {
        float side = 0.5f;
        // upper face
        vertices[0].Attr<dvL::Position3D>() = XMFLOAT3{ -side, side, side };  vertices[0].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        0.0f };
        vertices[1].Attr<dvL::Position3D>() = XMFLOAT3{ side, side, side };  vertices[1].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  0.0f };
        vertices[2].Attr<dvL::Position3D>() = XMFLOAT3{ -side, side,-side };  vertices[2].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        numTiled.y };
        vertices[3].Attr<dvL::Position3D>() = XMFLOAT3{ side, side,-side };  vertices[3].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  numTiled.y };
        // front face
        vertices[4].Attr<dvL::Position3D>() = XMFLOAT3{ -side, side,-side };  vertices[4].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        0.0f };
        vertices[5].Attr<dvL::Position3D>() = XMFLOAT3{ side, side,-side };  vertices[5].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  0.0f };
        vertices[6].Attr<dvL::Position3D>() = XMFLOAT3{ -side,-side,-side };  vertices[6].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        1.0f };
        vertices[7].Attr<dvL::Position3D>() = XMFLOAT3{ side,-side,-side };  vertices[7].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  1.0f };
        // down face
        vertices[8].Attr<dvL::Position3D>() = XMFLOAT3{ -side,-side,-side };  vertices[8].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        0.0f };
        vertices[9].Attr<dvL::Position3D>() = XMFLOAT3{ side,-side,-side };  vertices[9].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  0.0f };
        vertices[10].Attr<dvL::Position3D>() = XMFLOAT3{ -side,-side, side };  vertices[10].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        1.0f };
        vertices[11].Attr<dvL::Position3D>() = XMFLOAT3{ side,-side, side };  vertices[11].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  1.0f };
        // back face
        vertices[12].Attr<dvL::Position3D>() = XMFLOAT3{ side, side, side };  vertices[12].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        0.0f };
        vertices[13].Attr<dvL::Position3D>() = XMFLOAT3{ -side, side, side };  vertices[13].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  0.0f };
        vertices[14].Attr<dvL::Position3D>() = XMFLOAT3{ side,-side, side };  vertices[14].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        numTiled.y };
        vertices[15].Attr<dvL::Position3D>() = XMFLOAT3{ -side,-side, side };  vertices[15].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,  numTiled.y };
        // left face
        vertices[16].Attr<dvL::Position3D>() = XMFLOAT3{ -side, side, side };  vertices[16].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        0.0f };
        vertices[17].Attr<dvL::Position3D>() = XMFLOAT3{ -side, side,-side };  vertices[17].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,  0.0f };
        vertices[18].Attr<dvL::Position3D>() = XMFLOAT3{ -side,-side, side };  vertices[18].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        1.0f };
        vertices[19].Attr<dvL::Position3D>() = XMFLOAT3{ -side,-side,-side };  vertices[19].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,  1.0f };
        // right face
        vertices[20].Attr<dvL::Position3D>() = XMFLOAT3{ side, side,-side };  vertices[20].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        0.0f };
        vertices[21].Attr<dvL::Position3D>() = XMFLOAT3{ side, side, side };  vertices[21].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,  0.0f };
        vertices[22].Attr<dvL::Position3D>() = XMFLOAT3{ side,-side,-side };  vertices[22].Attr<dvL::Texture2D>() = XMFLOAT2{ 0.0f,        1.0f };
        vertices[23].Attr<dvL::Position3D>() = XMFLOAT3{ side,-side, side };  vertices[23].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,  1.0f };
    }

    std::vector<unsigned short> indices;
    for (int i = 0; i < 6; i++)
    {
        indices.push_back(i * 4 + 0); indices.push_back(i * 4 + 1); indices.push_back(i * 4 + 2);
        indices.push_back(i * 4 + 1); indices.push_back(i * 4 + 3); indices.push_back(i * 4 + 2);
    }

    IndexedTriangleList model(vertices, indices);
    model.Transform(XMMatrixScaling(size.x, size.y, size.z));
    model.SetNormalsIndependentFlat();

    pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
    pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
    pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto tcb = std::make_shared<TransformCbuf>(gfx);
    auto ttcb = std::make_shared<TutorialTransitionCbuf>(gfx);

    {
        Technique shade("Shade", Chan::main);
        {
            Step top("lambertianTrans");
            top.AddBindable(Sampler::Resolve(gfx));

            auto pvs = VertexShader::Resolve(gfx, "CubeTiled_VS.cso");
            top.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
            top.AddBindable(std::move(pvs));

            top.AddBindable(PixelShader::Resolve(gfx, "CubeTiledTutorial_PS.cso"));

            Dcb::RawLayout lay;
            lay.Add<Dcb::Float3>("specularColor");
            lay.Add<Dcb::Float>("specularWeight");
            lay.Add<Dcb::Float>("specularGloss");
            auto buf = Dcb::Buffer(std::move(lay));
            buf["specularColor"] = XMFLOAT3{ 1.0f,1.0f,1.0f };
            buf["specularWeight"] = 0.1f;
            buf["specularGloss"] = 20.0f;
            top.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

            top.AddBindable(Rasterizer::Resolve(gfx, false));
            top.AddBindable(Blender::Resolve(gfx, true));
            top.AddBindable(tcb);
            top.AddBindable(ttcb);
            top.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\block_noise_cloud.png", 1u));

            shade.AddStep(std::move(top));
        }
        AddTechnique(std::move(shade));
    }

    // Collapse
    {
        Technique collapse("Collapse", Chan::main, false);  // startActive = false
        {
            Step top("lambertianTrans");
            top.AddBindable(Sampler::Resolve(gfx));

            auto pvs = VertexShader::Resolve(gfx, "CubeTiled_VS.cso");
            top.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
            top.AddBindable(std::move(pvs));

            top.AddBindable(PixelShader::Resolve(gfx, "CubeTiledTutorial_Collapse_PS.cso"));

            Dcb::RawLayout lay;
            lay.Add<Dcb::Float3>("specularColor");
            lay.Add<Dcb::Float>("specularWeight");
            lay.Add<Dcb::Float>("specularGloss");
            auto buf = Dcb::Buffer(std::move(lay));
            buf["specularColor"] = XMFLOAT3{ 1.0f,1.0f,1.0f };
            buf["specularWeight"] = 0.1f;
            buf["specularGloss"] = 20.0f;
            top.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

            top.AddBindable(Rasterizer::Resolve(gfx, false));
            top.AddBindable(Blender::Resolve(gfx, true));
            top.AddBindable(tcb);
            top.AddBindable(ttcb);
            top.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\block_noise_cloud.png", 1u));

            collapse.AddStep(std::move(top));
        }
        AddTechnique(std::move(collapse));
    }
    // Grid
    {
        Technique grid("Grid", Chan::main);
        {
            Step top("lambertianTrans");
            top.AddBindable(Sampler::Resolve(gfx));

            auto pvs = VertexShader::Resolve(gfx, "CubeTiled_VS.cso");
            top.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
            top.AddBindable(std::move(pvs));

            top.AddBindable(PixelShader::Resolve(gfx, "CubeTiledTutorial_Grid_PS.cso"));

            top.AddBindable(Blender::Resolve(gfx, Blender::Mode::Additive));
            top.AddBindable(Rasterizer::Resolve(gfx, false));
            top.AddBindable(tcb);
            top.AddBindable(ttcb);
            top.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\block_noise_cloud.png", 1u));

            grid.AddStep(std::move(top));
        }
        AddTechnique(std::move(grid));
    }
    // Outline Mask
    {
        Technique outline("Outline", Chan::main);
        {
            Step mask("outlineMask");

            mask.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *VertexShader::Resolve(gfx, "Solid_VS.cso")));
            mask.AddBindable(NullPixelShader::Resolve(gfx));

            mask.AddBindable(std::move(tcb));

            outline.AddStep(std::move(mask));
        }
        AddTechnique(std::move(outline));
    }
}

// ----------------------------------------------------------------
// Transform
// ----------------------------------------------------------------
void CubeTiledTutorial::SetPosition(XMFLOAT3 pos) noexcept 
{ 
    trans.SetPosition(pos.x, pos.y, pos.z); 
}
void CubeTiledTutorial::SetRotation(float roll, float pitch, float yaw) noexcept 
{ 
    trans.SetRotationDegreeToRad(roll, pitch, yaw); 
}
void CubeTiledTutorial::SetScale(XMFLOAT3 size) noexcept 
{ 
    trans.SetScale(size.x, size.y, size.z); 
}
XMMATRIX CubeTiledTutorial::GetTransformXM() const noexcept 
{ 
    return trans.GetTransformXM(); 
}

// ----------------------------------------------------------------
// Update
// ----------------------------------------------------------------
void CubeTiledTutorial::Update(float dt)
{
    // Grid
    m_gridTimeAccumulator += dt;
    FT.numTiles = m_numTiled;
    FT.totalTime = m_gridTimeAccumulator;

    // Collapse
    if (m_isCollapsing && !m_collapseFinished)
    {
        m_collapseElapsedTime += m_collapseSpeed * dt * 60.0f;
        m_totalTimeAccumulator += dt;

        float norm = std::clamp(m_collapseElapsedTime / m_collapseDuration, 0.0f, 1.0f);

        FT.isCollapsing = 1;
        FT.collapseTime = norm;

        float maxDist = sqrtf(m_size.x * m_size.x + m_size.z * m_size.z) * 0.5f;
        FT.trRadius = norm * maxDist;
        m_currentCollapseRadiusWorld = FT.trRadius;

        FT.vortexStrength = sinf(norm * 3.14159f) * m_maxVortexStrength;
        FT.rippleCount = 3.0f + norm * 5.0f;

        float rainbowCurve = sinf(norm * 3.14159f);
        if (norm < 0.2f) rainbowCurve = (norm / 0.2f) * sinf(0.2f * 3.14159f);
        FT.rainbowIntensity = rainbowCurve * m_maxRainbowIntensity;

        techniques[0].SetActiveState(false); // close Shade
        techniques[1].SetActiveState(true);  // close Collapse

        if (norm >= 1.0f)
        {
            m_collapseFinished = true;
            m_isCollapsing = false;
            FT.isCollapsing = 0;
        }
        return;
    }

    techniques[0].SetActiveState(true);  // open Shade
    techniques[1].SetActiveState(false); // open Collapse
}

// Collapse
void CubeTiledTutorial::StartCollapse()
{
    if (m_isCollapsing) return;
    m_isCollapsing = true;
    m_collapseFinished = false;
    m_collapseElapsedTime = 0.0f;
    m_totalTimeAccumulator = 0.0f;
    m_currentCollapseRadiusWorld = 0.0f;

    FT.isCollapsing = 1;
    FT.trRadius = 0.0f;
    FT.trSoftness = 2.0f;
    FT.collapseTime = 0.0f;
    FT.vortexStrength = 0.0f;
    FT.rippleCount = 6.0f;
    FT.rainbowIntensity = 0.0f;
    FT.numTiles = m_numTiled;
}

bool CubeTiledTutorial::IsInsideCollapseHole(XMFLOAT3 worldPos) const
{
    if (!m_isCollapsing && !m_collapseFinished) return false;
    if (m_collapseFinished) return true;

    float dx = worldPos.x;
    float dz = worldPos.z;
    float effectiveRadius = m_currentCollapseRadiusWorld - FT.trSoftness * 0.3f;
    if (effectiveRadius < 0.0f) effectiveRadius = 0.0f;
    return sqrtf(dx * dx + dz * dz) < effectiveRadius;
}

void CubeTiledTutorial::Reset()
{
    m_isCollapsing = false;
    m_collapseFinished = false;
    m_collapseElapsedTime = 0.0f;
    m_totalTimeAccumulator = 0.0f;
    m_currentCollapseRadiusWorld = 0.0f;
    m_gridTimeAccumulator = 0.0f;

    FT = TutorialTransitionTag::value_type{};
    FT.trSoftness = 40.0f;
    FT.numTiles = m_numTiled;

    techniques[0].SetActiveState(true);
    techniques[1].SetActiveState(false);
}

// IProvides
TutorialTransitionTag::value_type CubeTiledTutorial::Provide(TutorialTransitionTag) const noexcept
{
    auto data = FT;
    data.totalTime = m_gridTimeAccumulator;
    return data;
}