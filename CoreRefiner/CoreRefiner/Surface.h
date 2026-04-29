#pragma once
#include "Win.h"
#include "ExceptionBase.h"
#include <string>
#include <optional>
#include "ConditionalNoexcept.h"
#include <dxtex/DirectXTex.h>
using namespace DirectX;

class Surface
{
public:
	class Color
{
public:
	constexpr Color() : dword() {}
	constexpr Color(unsigned int dw)
		:
		dword(dw)
	{}
	constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
		:
		dword((a << 24u) | (r << 16u) | (g << 8u) | b)
	{}
	constexpr Color(unsigned char r, unsigned char g, unsigned char b)
		:
		dword((255u << 24u) | (r << 16u) | (g << 8u) | b)
	{}
	constexpr Color(Color col, unsigned char a)
		:
		Color((a << 24u) | (col.GetR() << 16u) | (col.GetG() << 8u) | col.GetB())
	{}
	constexpr Color(const Color& col)
		:
		dword(col.dword)
	{}
	Color& operator =(Color color)
	{
		dword = color.dword;
		return *this;
	}
	~Color() = default;
	constexpr unsigned char GetA() const
	{
		return dword >> 24u;
	}
	constexpr unsigned char GetR() const
	{
		return (dword >> 16u) & 0xFFu;
	}
	constexpr unsigned char GetG() const
	{
		return (dword >> 8u) & 0xFFu;
	}
	constexpr unsigned char GetB() const
	{
		return dword & 0xFFu;
	}
	void SetA(unsigned char a)
	{
		dword = (dword & 0xFFFFFFu) | (a << 24u);
	}
	void SetR(unsigned char r)
	{
		dword = (dword & 0xFF00FFFFu) | (r << 16u);
	}
	void SetG(unsigned char g)
	{
		dword = (dword & 0xFFFF00FFu) | (g << 8u);
	}
	void SetB(unsigned char b)
	{
		dword = (dword & 0xFFFFFF00u) | b;
	}
public://演算子のオーバーロード
	friend inline const Color operator +(const Color& c1, const Color& c2)
	{
		return Color(
			static_cast<unsigned char>(std::min(255, c1.GetR() + c2.GetR())),
			static_cast<unsigned char>(std::min(255, c1.GetG() + c2.GetG())),
			static_cast<unsigned char>(std::min(255, c1.GetB() + c2.GetB())),
			static_cast<unsigned char>(std::min(255, c1.GetA() + c2.GetA()))
		);
	}
	friend inline const Color operator -(const Color& c1, const Color& c2)
	{
		return Color(
			static_cast<unsigned char>(std::max(0, c1.GetR() - c2.GetR())),
			static_cast<unsigned char>(std::max(0, c1.GetG() - c2.GetG())),
			static_cast<unsigned char>(std::max(0, c1.GetB() - c2.GetB())),
			static_cast<unsigned char>(std::max(0, c1.GetA() - c2.GetA()))
		);
	}
	friend inline const Color operator *(const Color& c1, const Color& c2)
	{
		return Color(
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetR()) * c2.GetR() / 255.0f)),
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetG()) * c2.GetG() / 255.0f)),
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetB()) * c2.GetB() / 255.0f)),
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetA()) * c2.GetA() / 255.0f))
		);
	}
	friend inline const Color operator *(const Color& c1, float k)
	{
		return Color(
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetR()) * k)),
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetG()) * k)),
			static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c1.GetB()) * k)),
			255u
		);
	}
	friend inline const Color operator *(float k, const Color& c1)
	{
		return c1 * k;
	}
	friend inline const Color operator /(const Color& c1, float k)   { if (k == 0) return c1; return c1 * (1.0f / k); }
	friend inline const Color operator+=(Color& c1, const Color& c2) { c1 = c1 + c2; return c1; }
	friend inline const Color operator-=(Color& c1, const Color& c2) { c1 = c1 - c2; return c1; }
	friend inline const Color operator*=(Color& c1, const Color& c2) { c1 = c1 * c2; return c1; }
	friend inline const Color operator*=(Color& c1, float k)		 { c1 = c1 * k; return c1; }
	friend inline const Color operator/=(Color& c1, float k)		 { c1 = c1 / k; return c1; }
	friend inline const bool  operator==(const Color& c1, const Color& c2) { return c1.dword == c2.dword; }
	friend inline const bool  operator!=(const Color& c1, const Color& c2) { return !(c1 == c2); }
	inline const void Normalize()
	{
		SetR(std::clamp(GetR(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
		SetG(std::clamp(GetG(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
		SetB(std::clamp(GetB(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
		SetA(std::clamp(GetA(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
	}
	inline DirectX::XMFLOAT4 GetNormalized() const noexcept
	{
		constexpr float inv255 = 1.0f / 255.0f;
		return DirectX::XMFLOAT4(
			GetR() * inv255,
			GetG() * inv255,
			GetB() * inv255,
			GetA() * inv255
		);
	}
public:
	unsigned int dword;
};
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

namespace Colors
{
	using Color = Surface::Color;
	static constexpr Color MakeRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		return (a << 24u) | (r << 16u) | (g << 8u) | b;
	}
	static constexpr Color MakeRGB(unsigned char r, unsigned char g, unsigned char b)
	{
		return MakeRGB(r, g, b, 255u);
	}
	static constexpr Color White	 =	MakeRGB(255u,	255u,	255u,	255u);
	static constexpr Color Black	 =	MakeRGB(0u,		0u,		0u,		255u);
	static constexpr Color Gray		 =	MakeRGB(211u,	211u,	211u,	255u);
	static constexpr Color LightGray =	MakeRGB(128u,	128u,	128u,	255u);
	static constexpr Color LightGray2=	MakeRGB(50u,	50u,	50u,	255u);
	static constexpr Color Red		 =	MakeRGB(255u,	0u,		0u,		255u);
	static constexpr Color Green	 =	MakeRGB(0u,		255u,	0u,		255u);
	static constexpr Color Blue		 =	MakeRGB(0u,		0u,		255u,	255u);
	static constexpr Color Yellow	 =	MakeRGB(255u,	255u,	0u,		255u);
	static constexpr Color Cyan		 =	MakeRGB(0u,		255u,	255u,	255u);
}