#pragma once
#include <DirectXMath.h>

using namespace DirectX;

XMFLOAT3 ExtractEulerAngles(const XMFLOAT4X4& matrix);
XMFLOAT3 ExtractTranslation(const XMFLOAT4X4& matrix);
XMMATRIX ScaleTranslation(FXMMATRIX matrix, float scale);

bool Normalize3(XMFLOAT3& v, float eps = 1e-8f) noexcept;
bool NormalizeXZ(XMFLOAT3& v, float eps = 1e-8f) noexcept;

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;

	Vec2() = default;
	constexpr Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}
	Vec2(XMFLOAT2 v) noexcept : x(v.x), y(v.y) {}

	[[nodiscard]] XMFLOAT2 ToFloat2() const noexcept { return { x, y }; }
	operator XMFLOAT2() const noexcept { return ToFloat2(); }

	[[nodiscard]] float LengthSq() const noexcept
	{
		const XMVECTOR v = XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(this));
		return XMVectorGetX(XMVector2LengthSq(v));
	}

	[[nodiscard]] float Length() const noexcept
	{
		const XMVECTOR v = XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(this));
		return XMVectorGetX(XMVector2Length(v));
	}

	[[nodiscard]] Vec2 Normalized(float eps = 1e-8f) const noexcept
	{
		Vec2 out{ *this };
		out.Normalize(eps);
		return out;
	}

	bool Normalize(float eps = 1e-8f) noexcept
	{
		XMFLOAT2 tmp{ x, y };
		XMVECTOR v = XMLoadFloat2(&tmp);
		if (XMVectorGetX(XMVector2LengthSq(v)) <= eps)
		{
			x = y = 0.0f;
			return false;
		}
		v = XMVector2Normalize(v);
		XMStoreFloat2(&tmp, v);
		x = tmp.x;
		y = tmp.y;
		return true;
	}
};

struct Vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	Vec3() = default;
	constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}
	Vec3(XMFLOAT3 v) noexcept : x(v.x), y(v.y), z(v.z) {}

	[[nodiscard]] XMFLOAT3 ToFloat3() const noexcept { return { x, y, z }; }
	operator XMFLOAT3() const noexcept { return ToFloat3(); }

	[[nodiscard]] float LengthSq() const noexcept
	{
		const XMVECTOR v = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(this));
		return XMVectorGetX(XMVector3LengthSq(v));
	}

	[[nodiscard]] float Length() const noexcept
	{
		const XMVECTOR v = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(this));
		return XMVectorGetX(XMVector3Length(v));
	}

	[[nodiscard]] Vec3 Normalized(float eps = 1e-8f) const noexcept
	{
		Vec3 out{ *this };
		out.Normalize(eps);
		return out;
	}

	bool Normalize(float eps = 1e-8f) noexcept
	{
		XMFLOAT3 tmp{ x, y, z };
		XMVECTOR v = XMLoadFloat3(&tmp);
		if (XMVectorGetX(XMVector3LengthSq(v)) <= eps)
		{
			x = y = z = 0.0f;
			return false;
		}
		v = XMVector3Normalize(v);
		XMStoreFloat3(&tmp, v);
		x = tmp.x;
		y = tmp.y;
		z = tmp.z;
		return true;
	}

	[[nodiscard]] Vec3 NormalizedXZ(float eps = 1e-8f) const noexcept
	{
		Vec3 out{ *this };
		out.NormalizeXZ(eps);
		return out;
	}

	bool NormalizeXZ(float eps = 1e-8f) noexcept
	{
		XMFLOAT3 tmp{ x, 0.0f, z };
		XMVECTOR v = XMLoadFloat3(&tmp);
		if (XMVectorGetX(XMVector3LengthSq(v)) <= eps)
		{
			x = y = z = 0.0f;
			return false;
		}
		v = XMVector3Normalize(v);
		XMStoreFloat3(&tmp, v);
		x = tmp.x;
		y = 0.0f;
		z = tmp.z;
		return true;
	}
};

// --- Vec2 operators ---
[[nodiscard]] inline Vec2 operator+(Vec2 a, Vec2 b) noexcept
{
	XMFLOAT2 r{};
	XMStoreFloat2(&r, XMVectorAdd(
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&a)),
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&b))));
	return Vec2{ r };
}

[[nodiscard]] inline Vec2 operator-(Vec2 a, Vec2 b) noexcept
{
	XMFLOAT2 r{};
	XMStoreFloat2(&r, XMVectorSubtract(
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&a)),
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&b))));
	return Vec2{ r };
}

[[nodiscard]] inline Vec2 operator-(Vec2 v) noexcept
{
	return Vec2{ -v.x, -v.y };
}

[[nodiscard]] inline Vec2 operator*(Vec2 v, float s) noexcept
{
	XMFLOAT2 r{};
	XMStoreFloat2(&r, XMVectorScale(
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&v)), s));
	return Vec2{ r };
}

[[nodiscard]] inline Vec2 operator*(float s, Vec2 v) noexcept { return v * s; }

[[nodiscard]] inline Vec2 operator/(Vec2 v, float s) noexcept
{
	return v * (1.0f / s);
}

inline Vec2& operator+=(Vec2& a, Vec2 b) noexcept { a = a + b; return a; }
inline Vec2& operator-=(Vec2& a, Vec2 b) noexcept { a = a - b; return a; }
inline Vec2& operator*=(Vec2& a, float s) noexcept { a = a * s; return a; }
inline Vec2& operator/=(Vec2& a, float s) noexcept { a = a / s; return a; }

[[nodiscard]] inline bool operator==(Vec2 a, Vec2 b) noexcept
{
	return a.x == b.x && a.y == b.y;
}

[[nodiscard]] inline bool operator!=(Vec2 a, Vec2 b) noexcept { return !(a == b); }

[[nodiscard]] inline float Dot(Vec2 a, Vec2 b) noexcept
{
	return XMVectorGetX(XMVector2Dot(
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&a)),
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&b))));
}

[[nodiscard]] inline Vec2 Lerp(Vec2 a, Vec2 b, float t) noexcept
{
	XMFLOAT2 r{};
	XMStoreFloat2(&r, XMVectorLerp(
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&a)),
		XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&b)),
		t));
	return Vec2{ r };
}

// --- Vec3 operators ---
[[nodiscard]] inline Vec3 operator+(Vec3 a, Vec3 b) noexcept
{
	XMFLOAT3 r{};
	XMStoreFloat3(&r, XMVectorAdd(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&a)),
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&b))));
	return Vec3{ r };
}

[[nodiscard]] inline Vec3 operator-(Vec3 a, Vec3 b) noexcept
{
	XMFLOAT3 r{};
	XMStoreFloat3(&r, XMVectorSubtract(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&a)),
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&b))));
	return Vec3{ r };
}

[[nodiscard]] inline Vec3 operator-(Vec3 v) noexcept
{
	return Vec3{ -v.x, -v.y, -v.z };
}

[[nodiscard]] inline Vec3 operator*(Vec3 v, float s) noexcept
{
	XMFLOAT3 r{};
	XMStoreFloat3(&r, XMVectorScale(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&v)), s));
	return Vec3{ r };
}

[[nodiscard]] inline Vec3 operator*(float s, Vec3 v) noexcept { return v * s; }

[[nodiscard]] inline Vec3 operator/(Vec3 v, float s) noexcept
{
	return v * (1.0f / s);
}

inline Vec3& operator+=(Vec3& a, Vec3 b) noexcept { a = a + b; return a; }
inline Vec3& operator-=(Vec3& a, Vec3 b) noexcept { a = a - b; return a; }
inline Vec3& operator*=(Vec3& a, float s) noexcept { a = a * s; return a; }
inline Vec3& operator/=(Vec3& a, float s) noexcept { a = a / s; return a; }

[[nodiscard]] inline bool operator==(Vec3 a, Vec3 b) noexcept
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

[[nodiscard]] inline bool operator!=(Vec3 a, Vec3 b) noexcept { return !(a == b); }

[[nodiscard]] inline float Dot(Vec3 a, Vec3 b) noexcept
{
	return XMVectorGetX(XMVector3Dot(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&a)),
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&b))));
}

[[nodiscard]] inline Vec3 Cross(Vec3 a, Vec3 b) noexcept
{
	XMFLOAT3 r{};
	XMStoreFloat3(&r, XMVector3Cross(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&a)),
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&b))));
	return Vec3{ r };
}

[[nodiscard]] inline Vec3 Lerp(Vec3 a, Vec3 b, float t) noexcept
{
	XMFLOAT3 r{};
	XMStoreFloat3(&r, XMVectorLerp(
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&a)),
		XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&b)),
		t));
	return Vec3{ r };
}

// Wrap XMFLOAT2/3 as Vec for expression math without changing storage types.
[[nodiscard]] inline Vec2 V(XMFLOAT2 v) noexcept { return Vec2{ v }; }
[[nodiscard]] inline Vec3 V(XMFLOAT3 v) noexcept { return Vec3{ v }; }