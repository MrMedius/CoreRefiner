#include "CanvasTexture.h"
#include "Canvas.h"
#include "Graphics.h"
#include "GraphicsThrowMacros.h"
#include <cassert>
#include <cstdint>

namespace Bind
{
	CanvasTexture::CanvasTexture(Graphics& gfx, UINT slot)
		:
		slot(slot)
	{
		(void)gfx;
	}

	void CanvasTexture::InitializeParentReference(const Drawable& parent) noexcept
	{
		pCanvas = dynamic_cast<const Canvas*>(&parent);
	}

	std::unique_ptr<CloningBindable> CanvasTexture::Clone() const noexcept
	{
		return std::make_unique<CanvasTexture>(*this);
	}

	void CanvasTexture::EnsureTexture(Graphics& gfx, UINT width, UINT height)
	{
		INFOMAN(gfx);

		if (width == 0u || height == 0u)
			return;
		if (pTexture && texW == width && texH == height)
			return;

		pSrv.Reset();
		pTexture.Reset();

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1u;
		desc.ArraySize = 1u;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1u;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		GFX_THROW_INFO(GetDevice(gfx)->CreateTexture2D(&desc, nullptr, &pTexture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0u;
		srvDesc.Texture2D.MipLevels = 1u;
		GFX_THROW_INFO(GetDevice(gfx)->CreateShaderResourceView(pTexture.Get(), &srvDesc, &pSrv));

		texW = width;
		texH = height;
	}

	void CanvasTexture::UploadIfDirty(Graphics& gfx)
	{
		assert(pCanvas != nullptr);

		const UINT w = pCanvas->GetCanvasWidth();
		const UINT h = pCanvas->GetCanvasHeight();
		EnsureTexture(gfx, w, h);
		if (!pTexture)
			return;

		if (!pCanvas->IsGpuDirty())
			return;

		const Surface& s = pCanvas->GetSurface();
		Canvas* pMutableCanvas = const_cast<Canvas*>(pCanvas);

		// dirty-rect 路径：只上传变更区域
		if (pMutableCanvas->hasDirtyRect)
		{
			const unsigned l = pMutableCanvas->dirtyMinX;
			const unsigned t = pMutableCanvas->dirtyMinY;
			const unsigned r = pMutableCanvas->dirtyMaxX + 1u;
			const unsigned b = pMutableCanvas->dirtyMaxY + 1u;

			D3D11_BOX box = {};
			box.left = l;
			box.top = t;
			box.front = 0u;
			box.right = r;
			box.bottom = b;
			box.back = 1u;

			const uint8_t* pSrcBase = reinterpret_cast<const uint8_t*>(s.GetBufferPtrConst());
			const UINT srcPitch = s.GetBytePitch();
			const uint8_t* pSrcOffset = pSrcBase + srcPitch * t + l * sizeof(Color);

			GetContext(gfx)->UpdateSubresource(
				pTexture.Get(),
				0u,
				&box,
				pSrcOffset,
				srcPitch,
				0u
			);
		}
		else
		{
			// 兜底全量
			GetContext(gfx)->UpdateSubresource(
				pTexture.Get(),
				0u,
				nullptr,
				s.GetBufferPtrConst(),
				s.GetBytePitch(),
				0u
			);
		}

		pMutableCanvas->ClearGpuDirty();
	}

	void CanvasTexture::Bind(Graphics& gfx) noxnd
	{
		assert(pCanvas != nullptr);
		UploadIfDirty(gfx);
		if (pSrv)
			GetContext(gfx)->PSSetShaderResources(slot, 1u, pSrv.GetAddressOf());
	}
}