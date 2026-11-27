#pragma once
#include "Bindable.h"
#include <memory>
#include <vector>
#include <string>

class Graphics;

namespace Bind
{
    class CubeTexture;

    class DynamicCubeTexture : public CloningBindable
    {
    public:
        DynamicCubeTexture(Graphics& gfx, std::vector<std::string> paths, UINT slot = 0u);

        void SetSlot(UINT s) noexcept { slot = s; }
        // 记下要显示的帧。frame 是帧号，张数不够时由 Bind 取模。
        void SetFrame(int frame) noexcept { frame_ = frame; }

        void Bind(Graphics& gfx) noxnd override;
        std::unique_ptr<CloningBindable> Clone() const noexcept override;

    private:
        // 当前要绑定的帧号。没有人改它时保持 0，和原来查不到提供者时一样。
        int frame_ = 0;
        UINT slot = 0u;
        std::vector<std::shared_ptr<Bind::CubeTexture>> frames;
    };
}
