#pragma once
#include "ConstantBuffers.h"
#include "Drawable.h"
#include <DirectXMath.h>
#include <memory>
#include "Provides.h"

namespace Bind
{
    class TutorialTransitionCbuf : public CloningBindable
    {
        using TutData = TutorialTransitionTag::value_type;
    public:
        TutorialTransitionCbuf(Graphics& gfx, UINT slot = 2u);
        void Bind(Graphics& gfx) noxnd override;
        void InitializeParentReference(const Drawable& parent) noexcept override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;
    private:
        void UpdateBindImpl(Graphics& gfx, const TutData& data) noxnd;
        TutData GetTutData() const noexcept;
    private:
        static std::unique_ptr<PixelConstantBuffer<TutData>> pPcbuf;
        const Drawable* pParent = nullptr;
    };
}
