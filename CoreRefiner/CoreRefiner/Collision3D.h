#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

namespace dx = DirectX;

namespace Collider3D
{
	// helper functions
    inline float Clamp(float v, float low, float high)
    {
        return std::clamp(v, low, high);
    }

    inline float Abs(float v) { return std::fabs(v); }

    inline float Dot3(dx::FXMVECTOR a, dx::FXMVECTOR b)
    {
        return dx::XMVectorGetX(dx::XMVector3Dot(a, b));
    }

    inline dx::XMVECTOR AbsV3(dx::FXMVECTOR v)
    {
        // abs for xyz, keep w as-is
        dx::XMVECTOR nv = dx::XMVectorNegate(v);
        return dx::XMVectorMax(v, nv);
    }

    inline float Length3(dx::FXMVECTOR v)
    {
        return dx::XMVectorGetX(dx::XMVector3Length(v));
    }

    inline float LengthSq3(dx::FXMVECTOR v)
    {
        return dx::XMVectorGetX(dx::XMVector3LengthSq(v));
    }

    inline dx::XMVECTOR Normalize3(dx::FXMVECTOR v)
    {
        // SafeNormalize: if length is tiny, return (1,0,0)
        dx::XMVECTOR lenSq = dx::XMVector3LengthSq(v);
        float ls = dx::XMVectorGetX(lenSq);
        if (ls < 1e-12f) return dx::XMVectorSet(1.f, 0.f, 0.f, 0.f);
        return dx::XMVector3Normalize(v);
    }

    inline bool IsIdentityAxes(dx::FXMVECTOR ax, dx::FXMVECTOR ay, dx::FXMVECTOR az, float eps = 1e-4f)
    {
        auto n = [&](dx::FXMVECTOR v, float x, float y, float z)->bool
        {
            return Abs(dx::XMVectorGetX(v) - x) < eps &&
                   Abs(dx::XMVectorGetY(v) - y) < eps &&
                   Abs(dx::XMVectorGetZ(v) - z) < eps;
        };
        return n(ax, 1, 0, 0) && n(ay, 0, 1, 0) && n(az, 0, 0, 1);
    }

    // Project an OBB (center + axes + half) onto world axes to get its AABB half extents.
    // extWorld = |ax|*hx + |ay|*hy + |az|*hz (component-wise)
    inline dx::XMVECTOR ComputeAABBHalfFromAxes(dx::FXMVECTOR ax, dx::FXMVECTOR ay, dx::FXMVECTOR az, dx::FXMVECTOR halfXYZ)
    {
        dx::XMVECTOR hx = dx::XMVectorSplatX(halfXYZ);
        dx::XMVECTOR hy = dx::XMVectorSplatY(halfXYZ);
        dx::XMVECTOR hz = dx::XMVectorSplatZ(halfXYZ);

        dx::XMVECTOR ext = dx::XMVectorMultiply(AbsV3(ax), hx);
        ext = dx::XMVectorAdd(ext, dx::XMVectorMultiply(AbsV3(ay), hy));
        ext = dx::XMVectorAdd(ext, dx::XMVectorMultiply(AbsV3(az), hz));
        return ext; // xyz valid
    }

    // Determine if a 3x3 rotation matrix (rows ax,ay,az) is close to a signed permutation matrix
    // i.e., aligned to world axes (including 90-degree swaps), within tolerance.
    inline bool IsAxisAlignedAxes(dx::FXMVECTOR ax, dx::FXMVECTOR ay, dx::FXMVECTOR az, float eps = 1e-3f)
    {
        // For axis aligned: each axis should have one component with abs ~ 1, others ~ 0.
        auto axisOk = [&](dx::FXMVECTOR a)->bool
            {
                float x = Abs(dx::XMVectorGetX(a));
                float y = Abs(dx::XMVectorGetY(a));
                float z = Abs(dx::XMVectorGetZ(a));

                // find max component and ensure it's near 1, others near 0
                float m = std::max(x, std::max(y, z));
                if (m < 1.0f - eps) return false;

                // Sum of squares should be ~1; and non-max components should be small.
                float s2 = x * x + y * y + z * z;
                if (Abs(s2 - 1.0f) > 5.0f * eps) return false;

                // other components small
                float ox = (m == x) ? y + z : (m == y) ? x + z : x + y;
                return ox < 5.0f * eps;
            };

        if (!axisOk(ax) || !axisOk(ay) || !axisOk(az)) return false;

        // Also ensure axes are orthogonal (robust against numeric drift)
        if (Abs(Dot3(ax, ay)) > 5.0f * eps) return false;
        if (Abs(Dot3(ax, az)) > 5.0f * eps) return false;
        if (Abs(Dot3(ay, az)) > 5.0f * eps) return false;

        return true;
    }


	// 3D Collider Types
    enum class CollideType : uint8_t
    {
        Point = 0,
        Sphere,
        Box,
        Capsule,
        Count
    };

    class Collision3D
    {
    public:
        virtual ~Collision3D() = default;
        virtual CollideType GetType() const = 0;
    };

    // Colliders only save data
    class PointCollider final : public Collision3D
    {
    public:
        PointCollider() = default;
        explicit PointCollider(const dx::XMFLOAT3& p) : position(p) {}
        CollideType GetType() const override { return CollideType::Point; }

        dx::XMFLOAT3 position{};
    };

    class SphereCollider final : public Collision3D
    {
    public:
        SphereCollider() = default;
        SphereCollider(const dx::XMFLOAT3& c, float r) : center(c), radius(r) {}
        CollideType GetType() const override { return CollideType::Sphere; }

        dx::XMFLOAT3 center{};
        float radius{};
    };

    enum class BoxAABBKind : uint8_t
    {
        OBB = 0,        // general OBB
        IdentityAABB,   // no rotation
        NormalizedAABB  // axis-aligned (maybe 90?? swap/flip)
    };
    class BoxCollider final : public Collision3D
    {
    public:
        CollideType GetType() const override { return CollideType::Box; }

        dx::XMFLOAT3 center{};
        dx::XMFLOAT3 half{};
        dx::XMFLOAT3 axisX{ 1,0,0 };
        dx::XMFLOAT3 axisY{ 0,1,0 };
        dx::XMFLOAT3 axisZ{ 0,0,1 };

        BoxAABBKind aabbKind{ BoxAABBKind::IdentityAABB };

        static BoxCollider BuildFromWorldMatrix(const dx::XMFLOAT4X4& world, const dx::XMFLOAT3& localHalf, float axisAlignedEps = 1e-3f);

        const dx::XMFLOAT3 GetSize() { return dx::XMFLOAT3(half.x * 2.0f, half.y * 2.0f, half.z * 2.0f); }
    };

    // Capsule = segment (pointA?pointB) thickened by radius (Unity-style).
    // pointA/pointB are hemisphere centers; total height = |B-A| + 2*radius.
    class CapsuleCollider final : public Collision3D
    {
    public:
        CapsuleCollider() = default;
        CapsuleCollider(const dx::XMFLOAT3& a, const dx::XMFLOAT3& b, float r)
            :
            pointA(a), pointB(b), radius(r)
        {}
        CollideType GetType() const override { return CollideType::Capsule; }

        dx::XMFLOAT3 pointA{};
        dx::XMFLOAT3 pointB{};
        float radius{};
    };

	// Real Collision System
    using CollideFn = bool(*)(const Collision3D&, const Collision3D&);
    using SeparateFn = bool(*)(const Collision3D&, const Collision3D&, DirectX::XMFLOAT3&, float&);
    class CollisionSystem
    {
    public:
        static bool IsOverlap(const Collision3D& a, const Collision3D& b);
        // Polymorphic penetration resolve (parallel to IsOverlap).
        // outNormal Unit axis that pushes `a` out of `b`.
        // outDepth Penetration depth (>= 0). Apply: a.pos += outNormal * outDepth.
        // true when overlapping; false when separated or unsupported pair.
        static bool TrySeparate(
            const Collision3D& a,
            const Collision3D& b,
            DirectX::XMFLOAT3& outNormal,
            float& outDepth);
    private:
        static const std::array<std::array<CollideFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>& Table();
        static const std::array<std::array<SeparateFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>& SeparateTable();
    };

    // Intersection Types
    bool Intersect(const SphereCollider& a, const SphereCollider& b);
    bool Intersect(const SphereCollider& s, const PointCollider& p);
    bool Intersect(const PointCollider& p, const SphereCollider& s);
    bool Intersect(const PointCollider& p, const BoxCollider& b);
    bool Intersect(const BoxCollider& b, const PointCollider& p);
    bool Intersect(const SphereCollider& s, const BoxCollider& b);
    bool Intersect(const BoxCollider& b, const SphereCollider& s);
    bool Intersect(const BoxCollider& a, const BoxCollider& b);

    bool Intersect(const CapsuleCollider& a, const CapsuleCollider& b);
    bool Intersect(const CapsuleCollider& c, const SphereCollider& s);
    bool Intersect(const SphereCollider& s, const CapsuleCollider& c);
    bool Intersect(const CapsuleCollider& c, const PointCollider& p);
    bool Intersect(const PointCollider& p, const CapsuleCollider& c);
    bool Intersect(const CapsuleCollider& c, const BoxCollider& b);
    bool Intersect(const BoxCollider& b, const CapsuleCollider& c);

    // Typed penetration resolve. outNormal pushes the first argument out of the second.
    // Apply: first.pos += outNormal * outDepth.
    bool ComputeSeparation(const SphereCollider& a, const SphereCollider& b, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const SphereCollider& s, const PointCollider& p, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const PointCollider& p, const SphereCollider& s, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const PointCollider& p, const BoxCollider& b, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const BoxCollider& b, const PointCollider& p, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const SphereCollider& s, const BoxCollider& b, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const BoxCollider& b, const SphereCollider& s, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const BoxCollider& a, const BoxCollider& b, DirectX::XMFLOAT3& outNormal, float& outDepth);

    bool ComputeSeparation(const CapsuleCollider& a, const CapsuleCollider& b, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const CapsuleCollider& c, const SphereCollider& s, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const SphereCollider& s, const CapsuleCollider& c, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const CapsuleCollider& c, const PointCollider& p, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const PointCollider& p, const CapsuleCollider& c, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const CapsuleCollider& c, const BoxCollider& b, DirectX::XMFLOAT3& outNormal, float& outDepth);
    bool ComputeSeparation(const BoxCollider& b, const CapsuleCollider& c, DirectX::XMFLOAT3& outNormal, float& outDepth);
}