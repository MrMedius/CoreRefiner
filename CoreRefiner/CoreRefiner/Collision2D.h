#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

namespace dx = DirectX;


namespace Collider2D
{
	inline float Clamp(float v, float low, float high)
	{
		return std::clamp(v, low, high);
	}

	inline float Abs(float v) { return std::fabs(v); }

	inline float Dot2(dx::FXMVECTOR a, dx::FXMVECTOR b)
	{
		return dx::XMVectorGetX(dx::XMVector2Dot(a, b));
	}

	inline float Length2(dx::FXMVECTOR v)
	{
		return dx::XMVectorGetX(dx::XMVector2Length(v));
	}

	inline float LengthSq2(dx::FXMVECTOR v)
	{
		return dx::XMVectorGetX(dx::XMVector2LengthSq(v));
	}

	inline dx::XMVECTOR Normalize2(dx::FXMVECTOR v)
	{
		dx::XMVECTOR lenSq = dx::XMVector2LengthSq(v);
		float ls = dx::XMVectorGetX(lenSq);
		if (ls < 1e-12f)
		{
			return dx::XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}
		return dx::XMVector2Normalize(v);
	}

	enum class CollideType : uint8_t
	{
		Point = 0,
		Circle,
		Box,
		Count
	};

	class Collision2D
	{
	public:
		virtual ~Collision2D() = default;
		virtual CollideType GetType() const = 0;
	};

	class PointCollider final : public Collision2D
	{
	public:
		PointCollider() = default;
		explicit PointCollider(const dx::XMFLOAT2& p) : position(p) {}
		CollideType GetType() const override { return CollideType::Point; }

		dx::XMFLOAT2 position{};
	};

	class CircleCollider final : public Collision2D
	{
	public:
		CircleCollider() = default;
		CircleCollider(const dx::XMFLOAT2& c, float r) : center(c), radius(r) {}
		CollideType GetType() const override { return CollideType::Circle; }

		dx::XMFLOAT2 center{};
		float radius{};
	};

	class BoxCollider final : public Collision2D
	{
	public:
		BoxCollider() = default;
		BoxCollider(const dx::XMFLOAT2& c, const dx::XMFLOAT2& h) : center(c), half(h) {}
		CollideType GetType() const override { return CollideType::Box; }

		dx::XMFLOAT2 center{};
		dx::XMFLOAT2 half{};

		[[nodiscard]] dx::XMFLOAT2 GetSize() const noexcept
		{
			return dx::XMFLOAT2{ half.x * 2.0f, half.y * 2.0f };
		}

		static BoxCollider MakeCenteredSquare(float halfExtent) noexcept
		{
			return BoxCollider{
				dx::XMFLOAT2{ 0.0f, 0.0f },
				dx::XMFLOAT2{ halfExtent, halfExtent }
			};
		}
	};

	using CollideFn = bool(*)(const Collision2D&, const Collision2D&);
	using SeparateFn = bool(*)(const Collision2D&, const Collision2D&, dx::XMFLOAT2&, float&);

	class CollisionSystem
	{
	public:
		static bool IsOverlap(const Collision2D& a, const Collision2D& b);

		static bool TrySeparate(
			const Collision2D& a,
			const Collision2D& b,
			dx::XMFLOAT2& outNormal,
			float& outDepth);

	private:
		static const std::array<std::array<CollideFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>& Table();
		static const std::array<std::array<SeparateFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>& SeparateTable();
	};

	// Typed intersection
	bool Intersect(const CircleCollider& a, const CircleCollider& b);
	bool Intersect(const CircleCollider& c, const PointCollider& p);
	bool Intersect(const PointCollider& p, const CircleCollider& c);
	bool Intersect(const PointCollider& p, const BoxCollider& b);
	bool Intersect(const BoxCollider& b, const PointCollider& p);
	bool Intersect(const CircleCollider& c, const BoxCollider& b);
	bool Intersect(const BoxCollider& b, const CircleCollider& c);
	bool Intersect(const BoxCollider& a, const BoxCollider& b);

	// Typed Separation
	bool ComputeSeparation(const CircleCollider& a, const CircleCollider& b, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const CircleCollider& c, const PointCollider& p, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const PointCollider& p, const CircleCollider& c, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const PointCollider& p, const BoxCollider& b, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const BoxCollider& b, const PointCollider& p, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const CircleCollider& c, const BoxCollider& b, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const BoxCollider& b, const CircleCollider& c, dx::XMFLOAT2& outNormal, float& outDepth);
	bool ComputeSeparation(const BoxCollider& a, const BoxCollider& b, dx::XMFLOAT2& outNormal, float& outDepth);

	dx::XMFLOAT2 ClampPointToBox(const dx::XMFLOAT2& p, const BoxCollider& box) noexcept;
}
