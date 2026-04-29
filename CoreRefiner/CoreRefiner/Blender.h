#pragma once
#include "Bindable.h"
#include <array>
#include <optional>

namespace Bind
{
    class Blender : public Bindable
    {
    public:
        enum class Mode
        {
            Disabled,       // no blending
            Alpha,          // SrcAlpha / InvSrcAlpha
            Additive,       // One / One
            Premultiplied,  // One / InvSrcAlpha  (for premultiplied textures)
            Multiply,       // DestColor / Zero   (darken, usually for decals/fog tint)
            FactorAlpha     // BlendFactor / InvBlendFactor
        };

    public:
        Blender(Graphics& gfx, Mode mode, std::optional<float> factor = {});

        void Bind(Graphics& gfx) noxnd override;
        void SetFactor(float factor) noxnd; // only valid when factor exists
        float GetFactor() const noxnd;

        static std::shared_ptr<Blender> Resolve(Graphics& gfx, Mode mode, std::optional<float> factor = {});
        static std::string GenerateUID(Mode mode, std::optional<float> factor);
        std::string GetUID() const noexcept override;

        // blending=false -> Disabled
        // blending=true && !factor -> Alpha
        // blending=true && factor -> FactorAlpha
        static std::shared_ptr<Blender> Resolve(Graphics& gfx, bool blending, std::optional<float> factor = {});

    protected:
        Microsoft::WRL::ComPtr<ID3D11BlendState> pBlender;
        Mode mode = Mode::Disabled;
        std::optional<std::array<float, 4>> factors;
    };
}
