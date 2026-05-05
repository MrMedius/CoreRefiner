#pragma once
#include "Win.h"
#include "ExceptionBase.h"
#include <string>
#include <optional>
#include "ConditionalNoexcept.h"
#include <dxtex/DirectXTex.h>
#include "Colors.h"

using namespace DirectX;

class Surface
{
public:
public:
	class Exception : public ExceptionBase
	{
	public:
		Exception(int line, const char* file, std::string note, std::optional<HRESULT> hr = {}) noexcept;
		Exception(int line, const char* file, std::string filename, std::string note, std::optional<HRESULT> hr = {}) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		const std::string& GetNote() const noexcept;
	private:
		std::optional<HRESULT> hr;
		std::string note;
	};
public:
	Surface(unsigned int width, unsigned int height);
	Surface(Surface&& source) noexcept = default;
	Surface(Surface&) = delete;
	Surface& operator=(Surface&& donor) noexcept = default;
	Surface& operator=(const Surface&) = delete;
	~Surface() = default;
	void Clear(Color fillValue) noexcept;
	void PutPixel(unsigned int x, unsigned int y, Color c) noxnd;
	Color GetPixel(unsigned int x, unsigned int y) const noxnd;
	unsigned int GetWidth() const noexcept;
	unsigned int GetHeight() const noexcept;
	unsigned int GetBytePitch() const noexcept;
	Color* GetBufferPtr() noexcept;
	const Color* GetBufferPtr() const noexcept;
	const Color* GetBufferPtrConst() const noexcept;
	static Surface FromFile(const std::string& name);
	static Surface FromMemory(const void* data, size_t size);
	static Surface FromPixels(const void* pixels, unsigned int width, unsigned int height);
	void Save(const std::string& filename) const;
	bool AlphaLoaded() const noexcept;
private:
	Surface(DirectX::ScratchImage scratch) noexcept;
	static const DirectX::Image* LoadCheck(HRESULT hr, DirectX::ScratchImage& scratch) noexcept;
private:
	static constexpr DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	DirectX::ScratchImage scratch;
};