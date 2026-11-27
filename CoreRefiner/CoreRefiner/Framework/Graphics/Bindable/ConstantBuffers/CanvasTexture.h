#pragma once
#include "Bindable.h"
#include "GraphicsResource.h"
#include <wrl/client.h>

class Canvas;
class Graphics;

namespace Bind
{
	class CanvasTexture : public CloningBindable
	{
	public:
		explicit CanvasTexture(Graphics& gfx, UINT slot = 0u);
		void Bind(Graphics& gfx) noxnd override;
		void InitializeParentReference(const Drawable& parent) noexcept override;
		std::unique_ptr<CloningBindable> Clone() const noexcept override;

	private:
		void EnsureTexture(Graphics& gfx, UINT width, UINT height);
		void UploadIfDirty(Graphics& gfx);

	private:
		UINT slot = 0u;
		const Canvas* pCanvas = nullptr;
		UINT texW = 0u;
		UINT texH = 0u;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pSrv;
	};
}