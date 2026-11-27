#include "Texture.h"
#include "Surface.h"
#include "BindableCodex.h"

namespace Bind
{
	namespace wrl = Microsoft::WRL;

	Texture::Texture(Graphics& gfx, const std::string& path, UINT slot)
		:
		slot(slot),
		path(path)
	{
		// load surface
		const auto s = Surface::FromFile(path);
		CreateTextureFromSurface(gfx, s);
	}

	Texture::Texture(Graphics& gfx, const std::string& embeddedId, const aiTexture* pAiTexture, UINT slot)
		:
		slot(slot),
		path(embeddedId)
	{
		Surface s = [&]() {
			if (pAiTexture->mHeight == 0)
			{
				// Compression format (e.g. PNG/JPG, where mWidth is the size of the compressed data in bytes)
				return Surface::FromMemory(pAiTexture->pcData, static_cast<size_t>(pAiTexture->mWidth));
			}
			else
			{
				// Uncompressed format (BGRA8888, mWidth * mHeight is the actual pixel size)
				return Surface::FromPixels(pAiTexture->pcData, pAiTexture->mWidth, pAiTexture->mHeight);
			}
		}();

		CreateTextureFromSurface(gfx, s);
	}

	void Texture::CreateTextureFromSurface(Graphics& gfx, const Surface& s)
	{
		hasAlpha = s.AlphaLoaded();

		// create texture resource
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = s.GetWidth();
		textureDesc.Height = s.GetHeight();
		textureDesc.MipLevels = 0;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		wrl::ComPtr<ID3D11Texture2D> pTexture;
		GetDevice(gfx)->CreateTexture2D(&textureDesc, nullptr, &pTexture);

		// write image data into top mip level
		GetContext(gfx)->UpdateSubresource(
			pTexture.Get(), 0u, nullptr, s.GetBufferPtrConst(), s.GetWidth() * sizeof(Color), 0u
		);

		// create the resource view on the texture
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		GetDevice(gfx)->CreateShaderResourceView(pTexture.Get(), &srvDesc, &pTextureView);

		// generate the mip chain using the gpu rendering pipeline
		GetContext(gfx)->GenerateMips(pTextureView.Get());
	}

	void Texture::Bind(Graphics& gfx) noxnd
	{
		GetContext(gfx)->PSSetShaderResources(slot, 1u, pTextureView.GetAddressOf());
	}
	std::shared_ptr<Texture> Texture::Resolve(Graphics& gfx, const std::string& path, UINT slot)
	{
		return Codex::Resolve<Texture>(gfx, path, slot);
	}
	std::shared_ptr<Texture> Texture::Resolve(Graphics& gfx, const std::string& embeddedId, const aiTexture* pAiTexture, UINT slot)
	{
		return Codex::Resolve<Texture>(gfx, embeddedId, pAiTexture, slot);
	}
	std::string Texture::GenerateUID(const std::string& path, UINT slot)
	{
		using namespace std::string_literals;
		return typeid(Texture).name() + "#"s + path + "#" + std::to_string(slot);
	}
	std::string Texture::GenerateUID(const std::string& embeddedId, const aiTexture*, UINT slot)
	{
		using namespace std::string_literals;
		return typeid(Texture).name() + "#embedded#"s + embeddedId + "#" + std::to_string(slot);
	}
	std::string Texture::GetUID() const noexcept
	{
		return GenerateUID(path, slot);
	}
	bool Texture::HasAlpha() const noexcept
	{
		return hasAlpha;
	}
	UINT Texture::CalculateNumberOfMipLevels(UINT width, UINT height) noexcept
	{
		const float xSteps = std::ceil(log2((float)width));
		const float ySteps = std::ceil(log2((float)height));
		return (UINT)std::max(xSteps, ySteps);
	}
}