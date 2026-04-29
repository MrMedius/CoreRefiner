#pragma once
#include "ConstantBuffers.h"
#include "Drawable.h"
#include <DirectXMath.h>
#include <memory>
#include "Provides.h"

namespace Bind
{
    class FieldTransitionCbuf : public CloningBindable
    {
    private:
        using FieldData = FieldTransitionTag::value_type;
    public:
        FieldTransitionCbuf(Graphics& gfx, UINT slot = 2u);
        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    private:
        void UpdateBindImpl(Graphics& gfx, const FieldData& data) noxnd;
        FieldData GetFieldData() const noexcept;
    private:
        static std::unique_ptr<PixelConstantBuffer<FieldData>> pPcbuf;
        const Drawable* pParent = nullptr;
    };
}
