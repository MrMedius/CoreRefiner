#include "Collision2D.h"

namespace Collider2D
{
	bool Intersect(const CircleCollider& a, const CircleCollider& b)
	{
		dx::XMVECTOR ca = dx::XMLoadFloat2(&a.center);
		dx::XMVECTOR cb = dx::XMLoadFloat2(&b.center);
		dx::XMVECTOR d = dx::XMVectorSubtract(ca, cb);
		const float r = a.radius + b.radius;
		return LengthSq2(d) <= r * r;
	}

	bool Intersect(const CircleCollider& c, const PointCollider& p)
	{
		dx::XMVECTOR cc = dx::XMLoadFloat2(&c.center);
		dx::XMVECTOR pp = dx::XMLoadFloat2(&p.position);
		dx::XMVECTOR d = dx::XMVectorSubtract(pp, cc);
		return LengthSq2(d) <= c.radius * c.radius;
	}

	bool Intersect(const PointCollider& p, const CircleCollider& c)
	{
		return Intersect(c, p);
	}

	bool Intersect(const PointCollider& p, const BoxCollider& b)
	{
		return Abs(p.position.x - b.center.x) <= b.half.x
			&& Abs(p.position.y - b.center.y) <= b.half.y;
	}

	bool Intersect(const BoxCollider& b, const PointCollider& p)
	{
		return Intersect(p, b);
	}

	bool Intersect(const CircleCollider& c, const BoxCollider& b)
	{
		const float closestX = Clamp(c.center.x, b.center.x - b.half.x, b.center.x + b.half.x);
		const float closestY = Clamp(c.center.y, b.center.y - b.half.y, b.center.y + b.half.y);
		const float dx = c.center.x - closestX;
		const float dy = c.center.y - closestY;
		return (dx * dx + dy * dy) <= c.radius * c.radius;
	}

	bool Intersect(const BoxCollider& b, const CircleCollider& c)
	{
		return Intersect(c, b);
	}

	bool Intersect(const BoxCollider& a, const BoxCollider& b)
	{
		return Abs(a.center.x - b.center.x) <= (a.half.x + b.half.x)
			&& Abs(a.center.y - b.center.y) <= (a.half.y + b.half.y);
	}

	bool ComputeSeparation(
		const CircleCollider& a,
		const CircleCollider& b,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		dx::XMVECTOR ca = dx::XMLoadFloat2(&a.center);
		dx::XMVECTOR cb = dx::XMLoadFloat2(&b.center);
		dx::XMVECTOR d = dx::XMVectorSubtract(ca, cb);
		const float distSq = LengthSq2(d);
		const float r = a.radius + b.radius;
		if (distSq > r * r)
		{
			outNormal = {};
			outDepth = 0.0f;
			return false;
		}

		if (distSq < 1e-12f)
		{
			outNormal = { 1.0f, 0.0f };
			outDepth = r;
			return true;
		}

		const float dist = std::sqrt(distSq);
		outDepth = r - dist;
		dx::XMVECTOR n = dx::XMVectorScale(d, 1.0f / dist);
		dx::XMStoreFloat2(&outNormal, n);
		return true;
	}

	bool ComputeSeparation(
		const CircleCollider& c,
		const PointCollider& p,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		dx::XMVECTOR cc = dx::XMLoadFloat2(&c.center);
		dx::XMVECTOR pp = dx::XMLoadFloat2(&p.position);
		dx::XMVECTOR d = dx::XMVectorSubtract(cc, pp);
		const float distSq = LengthSq2(d);
		if (distSq > c.radius * c.radius)
		{
			outNormal = {};
			outDepth = 0.0f;
			return false;
		}

		if (distSq < 1e-12f)
		{
			outNormal = { 1.0f, 0.0f };
			outDepth = c.radius;
			return true;
		}

		const float dist = std::sqrt(distSq);
		outDepth = c.radius - dist;
		dx::XMVECTOR n = dx::XMVectorScale(d, 1.0f / dist);
		dx::XMStoreFloat2(&outNormal, n);
		return true;
	}

	bool ComputeSeparation(
		const PointCollider& p,
		const CircleCollider& c,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		if (!ComputeSeparation(c, p, outNormal, outDepth))
		{
			return false;
		}
		outNormal.x = -outNormal.x;
		outNormal.y = -outNormal.y;
		return true;
	}

	bool ComputeSeparation(
		const PointCollider& p,
		const BoxCollider& b,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		const float dx = Abs(p.position.x - b.center.x);
		const float dy = Abs(p.position.y - b.center.y);
		if (dx > b.half.x || dy > b.half.y)
		{
			outNormal = {};
			outDepth = 0.0f;
			return false;
		}

		const float penX = b.half.x - dx;
		const float penY = b.half.y - dy;
		if (penX < penY)
		{
			outDepth = penX;
			outNormal = { (p.position.x >= b.center.x) ? 1.0f : -1.0f, 0.0f };
		}
		else
		{
			outDepth = penY;
			outNormal = { 0.0f, (p.position.y >= b.center.y) ? 1.0f : -1.0f };
		}
		return true;
	}

	bool ComputeSeparation(
		const BoxCollider& b,
		const PointCollider& p,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		if (!ComputeSeparation(p, b, outNormal, outDepth))
		{
			return false;
		}
		outNormal.x = -outNormal.x;
		outNormal.y = -outNormal.y;
		return true;
	}

	bool ComputeSeparation(
		const CircleCollider& c,
		const BoxCollider& b,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		const float closestX = Clamp(c.center.x, b.center.x - b.half.x, b.center.x + b.half.x);
		const float closestY = Clamp(c.center.y, b.center.y - b.half.y, b.center.y + b.half.y);
		float dx = c.center.x - closestX;
		float dy = c.center.y - closestY;
		float distSq = dx * dx + dy * dy;

		// Center inside box: separate along least penetration axis, then add radius.
		if (distSq < 1e-12f)
		{
			const float penX = b.half.x - Abs(c.center.x - b.center.x);
			const float penY = b.half.y - Abs(c.center.y - b.center.y);
			if (penX < penY)
			{
				outDepth = penX + c.radius;
				outNormal = { (c.center.x >= b.center.x) ? 1.0f : -1.0f, 0.0f };
			}
			else
			{
				outDepth = penY + c.radius;
				outNormal = { 0.0f, (c.center.y >= b.center.y) ? 1.0f : -1.0f };
			}
			return true;
		}

		const float dist = std::sqrt(distSq);
		if (dist > c.radius)
		{
			outNormal = {};
			outDepth = 0.0f;
			return false;
		}

		outDepth = c.radius - dist;
		outNormal = { dx / dist, dy / dist };
		return true;
	}

	bool ComputeSeparation(
		const BoxCollider& b,
		const CircleCollider& c,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		if (!ComputeSeparation(c, b, outNormal, outDepth))
		{
			return false;
		}
		outNormal.x = -outNormal.x;
		outNormal.y = -outNormal.y;
		return true;
	}

	bool ComputeSeparation(
		const BoxCollider& a,
		const BoxCollider& b,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		const float dx = (a.half.x + b.half.x) - Abs(a.center.x - b.center.x);
		const float dy = (a.half.y + b.half.y) - Abs(a.center.y - b.center.y);
		if (dx < 0.0f || dy < 0.0f)
		{
			outNormal = {};
			outDepth = 0.0f;
			return false;
		}

		if (dx < dy)
		{
			outDepth = dx;
			outNormal = { (a.center.x >= b.center.x) ? 1.0f : -1.0f, 0.0f };
		}
		else
		{
			outDepth = dy;
			outNormal = { 0.0f, (a.center.y >= b.center.y) ? 1.0f : -1.0f };
		}
		return true;
	}

	dx::XMFLOAT2 ClampPointToBox(const dx::XMFLOAT2& p, const BoxCollider& box) noexcept
	{
		return dx::XMFLOAT2{
			Clamp(p.x, box.center.x - box.half.x, box.center.x + box.half.x),
			Clamp(p.y, box.center.y - box.half.y, box.center.y + box.half.y)
		};
	}

	template<class A, class B>
	static bool Dispatch(const Collision2D& a, const Collision2D& b)
	{
		return Intersect(static_cast<const A&>(a), static_cast<const B&>(b));
	}

	template<class A, class B>
	static bool DispatchSeparate(
		const Collision2D& a,
		const Collision2D& b,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		return ComputeSeparation(static_cast<const A&>(a), static_cast<const B&>(b), outNormal, outDepth);
	}

	const std::array<std::array<CollideFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>&
		CollisionSystem::Table()
	{
		static std::array<std::array<CollideFn, (size_t)CollideType::Count>, (size_t)CollideType::Count> tbl = []
		{
			decltype(tbl) t{};
			for (auto& row : t)
			{
				row.fill(nullptr);
			}

			auto set = [&](CollideType A, CollideType B, CollideFn fnAB, CollideFn fnBA = nullptr)
			{
				t[(size_t)A][(size_t)B] = fnAB;
				t[(size_t)B][(size_t)A] = fnBA ? fnBA : fnAB;
			};

			set(CollideType::Circle, CollideType::Circle, &Dispatch<CircleCollider, CircleCollider>);
			set(CollideType::Circle, CollideType::Point, &Dispatch<CircleCollider, PointCollider>, &Dispatch<PointCollider, CircleCollider>);
			set(CollideType::Point, CollideType::Box, &Dispatch<PointCollider, BoxCollider>, &Dispatch<BoxCollider, PointCollider>);
			set(CollideType::Circle, CollideType::Box, &Dispatch<CircleCollider, BoxCollider>, &Dispatch<BoxCollider, CircleCollider>);
			set(CollideType::Box, CollideType::Box, &Dispatch<BoxCollider, BoxCollider>);

			return t;
		}();
		return tbl;
	}

	const std::array<std::array<SeparateFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>&
		CollisionSystem::SeparateTable()
	{
		static std::array<std::array<SeparateFn, (size_t)CollideType::Count>, (size_t)CollideType::Count> tbl = []
		{
			decltype(tbl) t{};
			for (auto& row : t)
			{
				row.fill(nullptr);
			}

			auto set = [&](CollideType A, CollideType B, SeparateFn fnAB, SeparateFn fnBA = nullptr)
			{
				t[(size_t)A][(size_t)B] = fnAB;
				t[(size_t)B][(size_t)A] = fnBA ? fnBA : fnAB;
			};

			set(CollideType::Circle, CollideType::Circle, &DispatchSeparate<CircleCollider, CircleCollider>);
			set(CollideType::Circle, CollideType::Point, &DispatchSeparate<CircleCollider, PointCollider>, &DispatchSeparate<PointCollider, CircleCollider>);
			set(CollideType::Point, CollideType::Box, &DispatchSeparate<PointCollider, BoxCollider>, &DispatchSeparate<BoxCollider, PointCollider>);
			set(CollideType::Circle, CollideType::Box, &DispatchSeparate<CircleCollider, BoxCollider>, &DispatchSeparate<BoxCollider, CircleCollider>);
			set(CollideType::Box, CollideType::Box, &DispatchSeparate<BoxCollider, BoxCollider>);

			return t;
		}();
		return tbl;
	}

	bool CollisionSystem::IsOverlap(const Collision2D& a, const Collision2D& b)
	{
		const auto ta = (size_t)a.GetType();
		const auto tb = (size_t)b.GetType();
		auto fn = Table()[ta][tb];
		return fn ? fn(a, b) : false;
	}

	bool CollisionSystem::TrySeparate(
		const Collision2D& a,
		const Collision2D& b,
		dx::XMFLOAT2& outNormal,
		float& outDepth)
	{
		const auto ta = (size_t)a.GetType();
		const auto tb = (size_t)b.GetType();
		auto fn = SeparateTable()[ta][tb];
		if (!fn)
		{
			outNormal = {};
			outDepth = 0.0f;
			return false;
		}
		return fn(a, b, outNormal, outDepth);
	}
}
