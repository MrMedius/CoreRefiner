#pragma once
#include "VertexBuffer.h"
#include "DynamicVertex.h"
#include "GraphicsThrowMacros.h"

namespace Bind
{
    class DynamicVertexBuffer : public VertexBuffer
    {
    public:
        // tag version, going into Codex
        DynamicVertexBuffer(Graphics& gfx, const std::string& tag, const Dvtx::VertexBuffer& vbuf);
        // no tag version
        DynamicVertexBuffer(Graphics& gfx, const Dvtx::VertexBuffer& vbuf);

        void Update(Graphics& gfx, const Dvtx::VertexBuffer& vbuf);

        // Override UID: Dynamic resources generally don't use Codex, so the UID isn't important; however, implementing this avoids pure virtual resources.
        std::string GetUID() const noexcept override;
    };
}
