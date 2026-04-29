#include "Collision.h"

namespace Collider3D
{
    // Box build
    BoxCollider BoxCollider::BuildFromWorldMatrix(const dx::XMFLOAT4X4& world, const dx::XMFLOAT3& localHalf, float axisAlignedEps)
    {
        BoxCollider b{};

        // center from _41,_42,_43 (matches your ExtractTranslation)
        b.center = dx::XMFLOAT3(world._41, world._42, world._43);

        // Extract basis vectors from ROWS (row-major)
        dx::XMVECTOR row0 = dx::XMVectorSet(world._11, world._12, world._13, 0.f); // Right (with scale)
        dx::XMVECTOR row1 = dx::XMVectorSet(world._21, world._22, world._23, 0.f); // Up    (with scale)
        dx::XMVECTOR row2 = dx::XMVectorSet(world._31, world._32, world._33, 0.f); // Forward(with scale)

        float sx = Length3(row0);
        float sy = Length3(row1);
        float sz = Length3(row2);

        dx::XMVECTOR ax = Normalize3(row0);
        dx::XMVECTOR ay = Normalize3(row1);
        dx::XMVECTOR az = Normalize3(row2);

        // Apply scale to half extents (localHalf is model-space half extents)
        b.half = dx::XMFLOAT3(localHalf.x * sx, localHalf.y * sy, localHalf.z * sz);

        // Store axes
        dx::XMStoreFloat3(&b.axisX, ax);
        dx::XMStoreFloat3(&b.axisY, ay);
        dx::XMStoreFloat3(&b.axisZ, az);

        const bool axisAligned = IsAxisAlignedAxes(ax, ay, az, axisAlignedEps);

        if (axisAligned)
        {
            if (IsIdentityAxes(ax, ay, az, 1e-4f))
            {
                b.aabbKind = BoxAABBKind::IdentityAABB;
                b.axisX = { 1,0,0 };
                b.axisY = { 0,1,0 };
                b.axisZ = { 0,0,1 };
            }
            else
            {
                dx::XMVECTOR halfLocalV = dx::XMLoadFloat3(&b.half);
                dx::XMVECTOR halfWorldV = ComputeAABBHalfFromAxes(ax, ay, az, halfLocalV);
                dx::XMStoreFloat3(&b.half, halfWorldV);

                b.axisX = { 1,0,0 };
                b.axisY = { 0,1,0 };
                b.axisZ = { 0,0,1 };


                b.aabbKind = BoxAABBKind::NormalizedAABB;
            }
        }
        else
        {
            b.aabbKind = BoxAABBKind::OBB;
        }

        return b;
    }

    // Intersection Types
    bool Intersect(const SphereCollider& a, const SphereCollider& b)
    {
        dx::XMVECTOR ca = dx::XMLoadFloat3(&a.center);
        dx::XMVECTOR cb = dx::XMLoadFloat3(&b.center);
        dx::XMVECTOR d = dx::XMVectorSubtract(ca, cb);

        float r = a.radius + b.radius;
        return LengthSq3(d) <= r * r;
    }

    bool Intersect(const SphereCollider& s, const PointCollider& p)
    {
        dx::XMVECTOR c = dx::XMLoadFloat3(&s.center);
        dx::XMVECTOR pt = dx::XMLoadFloat3(&p.position);
        dx::XMVECTOR d = dx::XMVectorSubtract(c, pt);
        return LengthSq3(d) <= s.radius * s.radius;
    }
    bool Intersect(const PointCollider& p, const SphereCollider& s)
    {
        return Intersect(s, p);
    }

    // Point vs Box: if axisAligned, treat as AABB in world axes; else OBB local test.
    bool Intersect(const PointCollider& p, const BoxCollider& b)
    {
        dx::XMVECTOR pt = dx::XMLoadFloat3(&p.position);
        dx::XMVECTOR c = dx::XMLoadFloat3(&b.center);
        dx::XMVECTOR d = dx::XMVectorSubtract(pt, c);

        if (b.aabbKind != BoxAABBKind::OBB)
        {
            dx::XMVECTOR ad = AbsV3(d);
            return (dx::XMVectorGetX(ad) <= b.half.x) &&
                   (dx::XMVectorGetY(ad) <= b.half.y) &&
                   (dx::XMVectorGetZ(ad) <= b.half.z);
        }

        // OBB local coordinates
        dx::XMVECTOR ax = dx::XMLoadFloat3(&b.axisX);
        dx::XMVECTOR ay = dx::XMLoadFloat3(&b.axisY);
        dx::XMVECTOR az = dx::XMLoadFloat3(&b.axisZ);

        float lx = Dot3(d, ax);
        float ly = Dot3(d, ay);
        float lz = Dot3(d, az);

        return (Abs(lx) <= b.half.x) && (Abs(ly) <= b.half.y) && (Abs(lz) <= b.half.z);
    }
    bool Intersect(const BoxCollider& b, const PointCollider& p)
    {
        return Intersect(p, b);
    }

    // Sphere vs Box: clamp closest point
    bool Intersect(const SphereCollider& s, const BoxCollider& b)
    {
        dx::XMVECTOR sc = dx::XMLoadFloat3(&s.center);
        dx::XMVECTOR c = dx::XMLoadFloat3(&b.center);
        dx::XMVECTOR d = dx::XMVectorSubtract(sc, c);

        if (b.aabbKind != BoxAABBKind::OBB)
        {
            float hx = b.half.x;
            float hy = b.half.y;
            float hz = b.half.z;

            float cx = dx::XMVectorGetX(sc);
            float cy = dx::XMVectorGetY(sc);
            float cz = dx::XMVectorGetZ(sc);

            float bx = dx::XMVectorGetX(c);
            float by = dx::XMVectorGetY(c);
            float bz = dx::XMVectorGetZ(c);

            float qx = Clamp(cx, bx - hx, bx + hx);
            float qy = Clamp(cy, by - hy, by + hy);
            float qz = Clamp(cz, bz - hz, bz + hz);

            dx::XMVECTOR q = dx::XMVectorSet(qx, qy, qz, 0.f);
            dx::XMVECTOR diff = dx::XMVectorSubtract(sc, q);
            return LengthSq3(diff) <= s.radius * s.radius;
        }

        // Sphere vs OBB: transform to OBB local, clamp, transform back
        dx::XMVECTOR ax = dx::XMLoadFloat3(&b.axisX);
        dx::XMVECTOR ay = dx::XMLoadFloat3(&b.axisY);
        dx::XMVECTOR az = dx::XMLoadFloat3(&b.axisZ);

        float lx = Dot3(d, ax);
        float ly = Dot3(d, ay);
        float lz = Dot3(d, az);

        float hx = b.half.x;
        float hy = b.half.y;
        float hz = b.half.z;

        float clx = Clamp(lx, -hx, hx);
        float cly = Clamp(ly, -hy, hy);
        float clz = Clamp(lz, -hz, hz);

        dx::XMVECTOR closest = dx::XMVectorAdd(dx::XMVectorScale(ay, cly), dx::XMVectorScale(az, clz));
        closest = dx::XMVectorAdd(dx::XMVectorScale(ax, clx), closest);
        closest = dx::XMVectorAdd(c, closest);

        dx::XMVECTOR diff = dx::XMVectorSubtract(sc, closest);
        return LengthSq3(diff) <= s.radius * s.radius;
    }
    bool Intersect(const BoxCollider& b, const SphereCollider& s)
    {
        return Intersect(s, b);
    }

    // OBB-OBB SAT helper (standard 15-axis test).
    // Uses floats for the core SAT math (stable & fast). Axes come from DirectXMath vectors.
    static bool IntersectOBB_OBB_SAT(const BoxCollider& A, const BoxCollider& B)
    {
        constexpr float EPS = 1e-6f;

        dx::XMVECTOR A0 = dx::XMLoadFloat3(&A.axisX);
        dx::XMVECTOR A1 = dx::XMLoadFloat3(&A.axisY);
        dx::XMVECTOR A2 = dx::XMLoadFloat3(&A.axisZ);

        dx::XMVECTOR B0 = dx::XMLoadFloat3(&B.axisX);
        dx::XMVECTOR B1 = dx::XMLoadFloat3(&B.axisY);
        dx::XMVECTOR B2 = dx::XMLoadFloat3(&B.axisZ);

        dx::XMVECTOR CA = dx::XMLoadFloat3(&A.center);
        dx::XMVECTOR CB = dx::XMLoadFloat3(&B.center);

        dx::XMVECTOR D = dx::XMVectorSubtract(CB, CA); // from A to B

        // Translation in A's frame
        float t[3] = { Dot3(D, A0), Dot3(D, A1), Dot3(D, A2) };

        // Rotation matrix R = Ai dot Bj
        float R[3][3] = {
            { Dot3(A0,B0), Dot3(A0,B1), Dot3(A0,B2) },
            { Dot3(A1,B0), Dot3(A1,B1), Dot3(A1,B2) },
            { Dot3(A2,B0), Dot3(A2,B1), Dot3(A2,B2) }
        };

        float AbsR[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                AbsR[i][j] = Abs(R[i][j]) + EPS;

        const float a[3] = { A.half.x, A.half.y, A.half.z };
        const float b[3] = { B.half.x, B.half.y, B.half.z };

        auto Sep = [&](float dist, float ra, float rb) -> bool
            {
                return Abs(dist) > (ra + rb);
            };

        // 1) Axes A0,A1,A2
        if (Sep(t[0], a[0], b[0] * AbsR[0][0] + b[1] * AbsR[0][1] + b[2] * AbsR[0][2])) return false;
        if (Sep(t[1], a[1], b[0] * AbsR[1][0] + b[1] * AbsR[1][1] + b[2] * AbsR[1][2])) return false;
        if (Sep(t[2], a[2], b[0] * AbsR[2][0] + b[1] * AbsR[2][1] + b[2] * AbsR[2][2])) return false;

        // 2) Axes B0,B1,B2 (translation in B frame)
        float tB[3] = {
            t[0] * R[0][0] + t[1] * R[1][0] + t[2] * R[2][0],
            t[0] * R[0][1] + t[1] * R[1][1] + t[2] * R[2][1],
            t[0] * R[0][2] + t[1] * R[1][2] + t[2] * R[2][2]
        };

        if (Sep(tB[0], a[0] * AbsR[0][0] + a[1] * AbsR[1][0] + a[2] * AbsR[2][0], b[0])) return false;
        if (Sep(tB[1], a[0] * AbsR[0][1] + a[1] * AbsR[1][1] + a[2] * AbsR[2][1], b[1])) return false;
        if (Sep(tB[2], a[0] * AbsR[0][2] + a[1] * AbsR[1][2] + a[2] * AbsR[2][2], b[2])) return false;

        // 3) 9 cross products Ai x Bj
        // A0 x B0
        if (Sep(t[2] * R[1][0] - t[1] * R[2][0],
            a[1] * AbsR[2][0] + a[2] * AbsR[1][0],
            b[1] * AbsR[0][2] + b[2] * AbsR[0][1])) return false;
        // A0 x B1
        if (Sep(t[2] * R[1][1] - t[1] * R[2][1],
            a[1] * AbsR[2][1] + a[2] * AbsR[1][1],
            b[0] * AbsR[0][2] + b[2] * AbsR[0][0])) return false;
        // A0 x B2
        if (Sep(t[2] * R[1][2] - t[1] * R[2][2],
            a[1] * AbsR[2][2] + a[2] * AbsR[1][2],
            b[0] * AbsR[0][1] + b[1] * AbsR[0][0])) return false;

        // A1 x B0
        if (Sep(t[0] * R[2][0] - t[2] * R[0][0],
            a[0] * AbsR[2][0] + a[2] * AbsR[0][0],
            b[1] * AbsR[1][2] + b[2] * AbsR[1][1])) return false;
        // A1 x B1
        if (Sep(t[0] * R[2][1] - t[2] * R[0][1],
            a[0] * AbsR[2][1] + a[2] * AbsR[0][1],
            b[0] * AbsR[1][2] + b[2] * AbsR[1][0])) return false;
        // A1 x B2
        if (Sep(t[0] * R[2][2] - t[2] * R[0][2],
            a[0] * AbsR[2][2] + a[2] * AbsR[0][2],
            b[0] * AbsR[1][1] + b[1] * AbsR[1][0])) return false;

        // A2 x B0
        if (Sep(t[1] * R[0][0] - t[0] * R[1][0],
            a[0] * AbsR[1][0] + a[1] * AbsR[0][0],
            b[1] * AbsR[2][2] + b[2] * AbsR[2][1])) return false;
        // A2 x B1
        if (Sep(t[1] * R[0][1] - t[0] * R[1][1],
            a[0] * AbsR[1][1] + a[1] * AbsR[0][1],
            b[0] * AbsR[2][2] + b[2] * AbsR[2][0])) return false;
        // A2 x B2
        if (Sep(t[1] * R[0][2] - t[0] * R[1][2],
            a[0] * AbsR[1][2] + a[1] * AbsR[0][2],
            b[0] * AbsR[2][1] + b[1] * AbsR[2][0])) return false;

        return true;
    }

    // Box vs Box: fast path AABB-AABB when both are axisAligned, otherwise SAT.
    bool Intersect(const BoxCollider& a, const BoxCollider& b)
    {
        if (a.aabbKind != BoxAABBKind::OBB && b.aabbKind != BoxAABBKind::OBB)
        {
            dx::XMVECTOR ca = dx::XMLoadFloat3(&a.center);
            dx::XMVECTOR cb = dx::XMLoadFloat3(&b.center);
            dx::XMVECTOR d = dx::XMVectorSubtract(ca, cb);
            dx::XMVECTOR ad = AbsV3(d);


            return (dx::XMVectorGetX(ad) <= (a.half.x + b.half.x)) &&
                   (dx::XMVectorGetY(ad) <= (a.half.y + b.half.y)) &&
                   (dx::XMVectorGetZ(ad) <= (a.half.z + b.half.z));
        }

        return IntersectOBB_OBB_SAT(a, b);
    }

    
    // Dispatch Table
    template<class A, class B>
    static bool Dispatch(const Collision3D& a, const Collision3D& b)
    {
        return Intersect(static_cast<const A&>(a), static_cast<const B&>(b));
    }

    const std::array<std::array<CollideFn, (size_t)CollideType::Count>, (size_t)CollideType::Count>& CollisionSystem::Table()
    {
        static std::array<std::array<CollideFn, (size_t)CollideType::Count>, (size_t)CollideType::Count> tbl = [] {
            decltype(tbl) t{};
            for (auto& row : t) row.fill(nullptr);

            auto set = [&](CollideType A, CollideType B, CollideFn fnAB, CollideFn fnBA = nullptr)
            {
                t[(size_t)A][(size_t)B] = fnAB;
                t[(size_t)B][(size_t)A] = fnBA ? fnBA : fnAB;
            };

            set(CollideType::Sphere, CollideType::Sphere, &Dispatch<SphereCollider, SphereCollider>);
            set(CollideType::Sphere, CollideType::Point,  &Dispatch<SphereCollider, PointCollider>, &Dispatch<PointCollider, SphereCollider>);
            set(CollideType::Point,  CollideType::Box,    &Dispatch<PointCollider, BoxCollider>,    &Dispatch<BoxCollider, PointCollider>);
            set(CollideType::Sphere, CollideType::Box,    &Dispatch<SphereCollider, BoxCollider>,   &Dispatch<BoxCollider, SphereCollider>);
            set(CollideType::Box,    CollideType::Box,    &Dispatch<BoxCollider, BoxCollider>);

            return t;
            }();
        return tbl;
    }

    bool CollisionSystem::IsOverlap(const Collision3D& a, const Collision3D& b)
    {
        const auto ta = (size_t)a.GetType();
        const auto tb = (size_t)b.GetType();
        auto fn = Table()[ta][tb];
        return fn ? fn(a, b) : false;
    }

}