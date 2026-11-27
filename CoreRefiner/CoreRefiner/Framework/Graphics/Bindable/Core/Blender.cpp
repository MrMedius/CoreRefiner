#include "Blender.h"
#include "GraphicsThrowMacros.h"
#include "BindableCodex.h"
#include <cassert>

namespace Bind
{
    Blender::Blender(Graphics& gfx, Mode modeIn, std::optional<float> factorIn)
        :
        mode(modeIn)
    {
        INFOMAN(gfx);

        if (factorIn)
        {
            factors.emplace();
            factors->fill(*factorIn);
        }

        D3D11_BLEND_DESC desc = CD3D11_BLEND_DESC{ CD3D11_DEFAULT{} };
        auto& rt = desc.RenderTarget[0];

        // Default : No blending
        rt.BlendEnable = FALSE;

        auto enable = [&]()
            {
                rt.BlendEnable = TRUE;
                rt.BlendOp = D3D11_BLEND_OP_ADD;
                rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
                rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            };

        switch (mode)
        {
        default:
        case Mode::Disabled:
            // BlendEnable = FALSE
            break;

        case Mode::Alpha:
            enable();
            rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
            rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            break;

        case Mode::Additive:
            enable();
            rt.SrcBlend = D3D11_BLEND_ONE;
            rt.DestBlend = D3D11_BLEND_ONE;
            rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlendAlpha = D3D11_BLEND_ONE;
            break;

        case Mode::Premultiplied:
            // texture rgb has multiplied alphaFdst = src*1 + dst*(1-srcA)
            enable();
            rt.SrcBlend = D3D11_BLEND_ONE;
            rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            break;

        case Mode::Multiply:
            // dst = src * dstColor  (Commonly used for darkening / stains / certain fogging tint)
            enable();
            rt.SrcBlend = D3D11_BLEND_DEST_COLOR;
            rt.DestBlend = D3D11_BLEND_ZERO;
            rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlendAlpha = D3D11_BLEND_ZERO;
            break;

        case Mode::FactorAlpha:
            // Use BlendFactor to control the mixing intensity
            // A factor must be provided, otherwise it's meaningless
            enable();
            assert(factors && "FactorAlpha mode requires factor");
            rt.SrcBlend = D3D11_BLEND_BLEND_FACTOR;
            rt.DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
            rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            break;
        }

        GFX_THROW_INFO(GetDevice(gfx)->CreateBlendState(&desc, &pBlender));
    }

    void Blender::Bind(Graphics& gfx) noxnd
    {
        INFOMAN_NOHR(gfx);
        const float* data = factors ? factors->data() : nullptr;
        GFX_THROW_INFO_ONLY(GetContext(gfx)->OMSetBlendState(pBlender.Get(), data, 0xFFFFFFFFu));
    }

    void Blender::SetFactor(float factor) noxnd
    {
        assert(factors && "SetFactor only valid when factor exists");
        factors->fill(factor);
    }

    float Blender::GetFactor() const noxnd
    {
        assert(factors && "GetFactor only valid when factor exists");
        return factors->front();
    }

    std::shared_ptr<Blender> Blender::Resolve(Graphics& gfx, Mode mode, std::optional<float> factor)
    {
        return Codex::Resolve<Blender>(gfx, mode, factor);
    }

    std::shared_ptr<Blender> Blender::Resolve(Graphics& gfx, bool blending, std::optional<float> factor)
    {
        if (!blending) return Resolve(gfx, Mode::Disabled, {});
        if (factor)     return Resolve(gfx, Mode::FactorAlpha, factor);
        return Resolve(gfx, Mode::Alpha, {});
    }

    std::string Blender::GenerateUID(Mode mode, std::optional<float> factor)
    {
        using namespace std::string_literals;
        return typeid(Blender).name() + "#"s + std::to_string((int)mode)
            + (factor ? "#f"s + std::to_string(*factor) : "");
    }

    std::string Blender::GetUID() const noexcept
    {
        return GenerateUID(mode, factors ? std::optional<float>{ factors->front() } : std::optional<float>{});
    }
}