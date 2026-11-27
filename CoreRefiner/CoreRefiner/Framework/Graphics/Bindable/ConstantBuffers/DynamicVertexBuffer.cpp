#include "DynamicVertexBuffer.h"

namespace Bind
{
    DynamicVertexBuffer::DynamicVertexBuffer(Graphics& gfx, const Dvtx::VertexBuffer& vbuf)
        : DynamicVertexBuffer(gfx, "dynvb", vbuf)
    {
    }

    DynamicVertexBuffer::DynamicVertexBuffer(Graphics& gfx, const std::string& tag, const Dvtx::VertexBuffer& vbuf)
        :
        VertexBuffer(gfx, tag, vbuf)
    {
        INFOMAN(gfx);

        D3D11_BUFFER_DESC bd = {};
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.MiscFlags = 0u;
        bd.ByteWidth = (UINT)vbuf.SizeBytes();
        bd.StructureByteStride = stride;

        // Initial data
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = vbuf.GetData();

        pVertexBuffer.Reset();
        GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&bd, &sd, &pVertexBuffer));
    }

    void DynamicVertexBuffer::Update(Graphics& gfx, const Dvtx::VertexBuffer& vbuf)
    {
        // layout must be consistent
        assert(vbuf.GetLayout().GetCode() == layout.GetCode());
        assert((UINT)vbuf.SizeBytes() <= (UINT)0x7fffffff);

        INFOMAN(gfx);

        D3D11_MAPPED_SUBRESOURCE msr{};
        GFX_THROW_INFO(GetContext(gfx)->Map(pVertexBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr));
        memcpy(msr.pData, vbuf.GetData(), vbuf.SizeBytes());
        GetContext(gfx)->Unmap(pVertexBuffer.Get(), 0u);
    }

    std::string DynamicVertexBuffer::GetUID() const noexcept
    {
        // Dynamic VB is not recommended to use Codex; UID is only used for debugging/probing.
        return std::string(typeid(DynamicVertexBuffer).name()) + "#" + tag;
    }
}
