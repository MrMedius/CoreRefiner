#include "Sprite3DNoLit.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include "SpriteUVCbuf.h"
#include "DynamicTexture.h"
#include "Channels.h"

Sprite3DNoLit::Sprite3DNoLit(Graphics& gfx, std::vector<std::string> paths)
{
	using namespace Bind;
	namespace dx = DirectX;

    auto model = Plane::Make2D(1, 1);
    const auto geometryTag = "$plane." + paths[0] + std::to_string(paths[0].size());
    pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
    pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
    pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto tcb = std::make_shared<TransformCbuf>(gfx);
    auto uvcb = std::make_shared<SpriteUVCbuf>(gfx);
    auto dynTex = std::make_shared<DynamicTexture>(gfx, paths);

    {
        Technique shade("Shade", Chan::main);
        Step only("lambertianTrans");

        only.AddBindable(dynTex);
        only.AddBindable(Sampler::Resolve(gfx));

        auto pvs = VertexShader::Resolve(gfx, "Sprite3DNoLit_VS.cso");
        only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
        only.AddBindable(std::move(pvs));

        only.AddBindable(PixelShader::Resolve(gfx, "Sprite3DNoLit_PS.cso"));

        only.AddBindable(Rasterizer::Resolve(gfx, true));
        only.AddBindable(Blender::Resolve(gfx, true));

        only.AddBindable(tcb);
        only.AddBindable(uvcb);

        shade.AddStep(std::move(only));
        AddTechnique(std::move(shade));
    }
    {
        Technique outline("Outline", Chan::main);
        Step mask("outlineMask");

        auto pvs = VertexShader::Resolve(gfx, "Sprite3D_Mask_VS.cso");
        mask.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
        mask.AddBindable(std::move(pvs));

        mask.AddBindable(dynTex);
        mask.AddBindable(Sampler::Resolve(gfx));
        mask.AddBindable(PixelShader::Resolve(gfx, "Sprite3D_Mask_PS.cso"));

        mask.AddBindable(tcb);
        mask.AddBindable(uvcb);

        outline.AddStep(std::move(mask));
        AddTechnique(std::move(outline));
    }
}