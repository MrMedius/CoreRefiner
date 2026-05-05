#pragma once
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <algorithm>

class Color
{
public:
	constexpr Color() : dword() {}
	constexpr Color(unsigned int dw)
		:
		dword(dw)
	{
	}
	constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
		:
		dword((a << 24u) | (r << 16u) | (g << 8u) | b)
	{
	}
	constexpr Color(unsigned char r, unsigned char g, unsigned char b)
		:
		dword((255u << 24u) | (r << 16u) | (g << 8u) | b)
	{
	}
	constexpr Color(Color col, unsigned char a)
		:
		Color((a << 24u) | (col.GetR() << 16u) | (col.GetG() << 8u) | col.GetB())
	{
	}
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
	void Normalize()
	{
		SetR(std::clamp(GetR(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
		SetG(std::clamp(GetG(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
		SetB(std::clamp(GetB(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
		SetA(std::clamp(GetA(), static_cast<unsigned char>(0), static_cast<unsigned char>(255)));
	}
public:
	unsigned int dword;
};

namespace Colors
{
	static constexpr Color MakeRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		return (a << 24u) | (r << 16u) | (g << 8u) | b;
	}
	static constexpr Color MakeRGB(unsigned char r, unsigned char g, unsigned char b)
	{
		return MakeRGB(r, g, b, 255u);
	}
	static constexpr Color None			=	MakeRGB(0u,		0u,		0u,		0u);
	static constexpr Color White		=	MakeRGB(255u,	255u,	255u,	255u);
	static constexpr Color Black		=	MakeRGB(0u,		0u,		0u,		255u);
	static constexpr Color Gray			=	MakeRGB(211u,	211u,	211u,	255u);
	static constexpr Color LightGray	=	MakeRGB(128u,	128u,	128u,	255u);
	static constexpr Color LightGray2	=	MakeRGB(50u,	50u,	50u,	255u);
	static constexpr Color Red			=	MakeRGB(255u,	0u,		0u,		255u);
	static constexpr Color Green		=	MakeRGB(0u,		255u,	0u,		255u);
	static constexpr Color Blue			=	MakeRGB(0u,		0u,		255u,	255u);
	static constexpr Color Yellow		=	MakeRGB(255u,	255u,	0u,		255u);
	static constexpr Color Cyan			=	MakeRGB(0u,		255u,	255u,	255u);
	static constexpr Color Bocchi		=	MakeRGB(255u,	192u,	230u,	255u);
	static constexpr Color Nijika		=	MakeRGB(255u,	200u,	0u,		255u);
	static constexpr Color Ryo			=	MakeRGB(2u,		209u,	224u,	255u);
	static constexpr Color Kita			=	MakeRGB(255u,	70u,	55u,	255u);
	static constexpr Color Chart[12][9] = { 
		{Color(0xFFFFCCCCu),Color(0xFFFF9999u),Color(0xFFFF6666u),Color(0xFFFF3333u),Color(0xFFFF0000u),Color(0xFFCC0000u),Color(0xFF990000u),Color(0xFF660000u),Color(0xFF330000u)},	//Red
		{Color(0xFFFFE5CCu),Color(0xFFFFCC99u),Color(0xFFFFB266u),Color(0xFFFF9933u),Color(0xFFFF8000u),Color(0xFFCC6600u),Color(0xFF994C00u),Color(0xFF663300u),Color(0xFF331900u)},	//Orange
		{Color(0xFFFFFFCCu),Color(0xFFFFFF99u),Color(0xFFFFFF66u),Color(0xFFFFFF33u),Color(0xFFFFFF00u),Color(0xFFCCCC00u),Color(0xFF999900u),Color(0xFF666600u),Color(0xFF333300u)},	//Yellow
		{Color(0xFFE5FFCCu),Color(0xFFCCFF99u),Color(0xFFB2FF66u),Color(0xFF99FF33u),Color(0xFF80FF00u),Color(0xFF66CC00u),Color(0xFF4C9900u),Color(0xFF336600u),Color(0xFF193300u)},	//LightGreen
		{Color(0xFFCCFFCCu),Color(0xFF99FF99u),Color(0xFF66FF66u),Color(0xFF66FF66u),Color(0xFF00FF00u),Color(0xFF00CC00u),Color(0xFF009900u),Color(0xFF006600u),Color(0xFF003300u)},	//Green
		{Color(0xFFCCFFE5u),Color(0xFF99FFCCu),Color(0xFF66FFB2u),Color(0xFF33FF99u),Color(0xFF00FF80u),Color(0xFF00CC66u),Color(0xFF00994Cu),Color(0xFF006633u),Color(0xFF003319u)},	//LightCyan
		{Color(0xFFCCFFFFu),Color(0xFF99FFFFu),Color(0xFF66FFFFu),Color(0xFF33FFFFu),Color(0xFF00FFFFu),Color(0xFF00CCCCu),Color(0xFF009999u),Color(0xFF006666u),Color(0xFF003333u)},	//Cyan
		{Color(0xFFCCE5FFu),Color(0xFF99CCFFu),Color(0xFF66B2FFu),Color(0xFF3399FFu),Color(0xFF0080FFu),Color(0xFF0066CCu),Color(0xFF004C99u),Color(0xFF003366u),Color(0xFF001933u)},	//LightBlue
		{Color(0xFFCCCCFFu),Color(0xFF9999FFu),Color(0xFF6666FFu),Color(0xFF3333FFu),Color(0xFF0000FFu),Color(0xFF0000CCu),Color(0xFF000099u),Color(0xFF000066u),Color(0xFF000033u)},	//Blue
		{Color(0xFFE5CCFFu),Color(0xFFCC99FFu),Color(0xFFB266FFu),Color(0xFF9933FFu),Color(0xFF7F00FFu),Color(0xFF6600CCu),Color(0xFF4C0099u),Color(0xFF330066u),Color(0xFF190033u)},	//Purple
		{Color(0xFFFFCCFFu),Color(0xFFFF99FFu),Color(0xFFFF66FFu),Color(0xFFFF33FFu),Color(0xFFFF00FFu),Color(0xFFCC00CCu),Color(0xFF990099u),Color(0xFF660066u),Color(0xFF330033u)},	//Magenta
		{Color(0xFFFFCCE5u),Color(0xFFFF99CCu),Color(0xFFFF66B2u),Color(0xFFFF3399u),Color(0xFFFF007Fu),Color(0xFFCC0066u),Color(0xFF99004Cu),Color(0xFF660033u),Color(0xFF330019u)}	//DeepPink
	};
										  
}