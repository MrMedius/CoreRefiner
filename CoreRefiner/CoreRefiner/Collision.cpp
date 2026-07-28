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

    // --- Capsule helpers / intersections ---

    /** @brief Closest point on segment AB to point P. */
    static dx::XMVECTOR ClosestPointOnSegment(dx::FXMVECTOR a, dx::FXMVECTOR b, dx::FXMVECTOR p)
    {
        dx::XMVECTOR ab = dx::XMVectorSubtract(b, a);
        float abLenSq = LengthSq3(ab);
        if (abLenSq < 1e-12f)
        {
            return a;
        }
        float t = Dot3(dx::XMVectorSubtract(p, a), ab) / abLenSq;
        t = Clamp(t, 0.0f, 1.0f);
        return dx::XMVectorAdd(a, dx::XMVectorScale(ab, t));
    }

    /**
     * @brief Closest points between segments AB and CD; returns squared distance.
     */
    static float ClosestPointsSegmentSegment(
        dx::FXMVECTOR a, dx::FXMVECTOR b,
        dx::FXMVECTOR c, dx::FXMVECTOR d,
        dx::XMVECTOR& outP, dx::XMVECTOR& outQ)
    {
        constexpr float EPS = 1e-8f;
        dx::XMVECTOR ab = dx::XMVectorSubtract(b, a);
        dx::XMVECTOR cd = dx::XMVectorSubtract(d, c);
        dx::XMVECTOR ac = dx::XMVectorSubtract(a, c);

        float abLenSq = LengthSq3(ab);
        float cdLenSq = LengthSq3(cd);
        float abDotCd = Dot3(ab, cd);
        float acDotAb = Dot3(ac, ab);
        float acDotCd = Dot3(ac, cd);

        float s = 0.0f;
        float t = 0.0f;

        if (abLenSq < EPS && cdLenSq < EPS)
        {
            outP = a;
            outQ = c;
            return LengthSq3(dx::XMVectorSubtract(outP, outQ));
        }
        if (abLenSq < EPS)
        {
            s = 0.0f;
            t = Clamp(acDotCd / cdLenSq, 0.0f, 1.0f);
        }
        else if (cdLenSq < EPS)
        {
            t = 0.0f;
            s = Clamp(-acDotAb / abLenSq, 0.0f, 1.0f);
        }
        else
        {
            float denom = abLenSq * cdLenSq - abDotCd * abDotCd;
            if (denom > EPS)
            {
                s = Clamp((abDotCd * acDotCd - cdLenSq * acDotAb) / denom, 0.0f, 1.0f);
            }
            else
            {
                s = 0.0f;
            }
            t = (abDotCd * s + acDotCd) / cdLenSq;
            if (t < 0.0f)
            {
                t = 0.0f;
                s = Clamp(-acDotAb / abLenSq, 0.0f, 1.0f);
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = Clamp((abDotCd - acDotAb) / abLenSq, 0.0f, 1.0f);
            }
        }

        outP = dx::XMVectorAdd(a, dx::XMVectorScale(ab, s));
        outQ = dx::XMVectorAdd(c, dx::XMVectorScale(cd, t));
        return LengthSq3(dx::XMVectorSubtract(outP, outQ));
    }

    /** @brief Closest point on AABB (axis-aligned box) to point P. */
    static dx::XMVECTOR ClosestPointOnAABB(dx::FXMVECTOR p, dx::FXMVECTOR center, float hx, float hy, float hz)
    {
        float px = dx::XMVectorGetX(p);
        float py = dx::XMVectorGetY(p);
        float pz = dx::XMVectorGetZ(p);
        float cx = dx::XMVectorGetX(center);
        float cy = dx::XMVectorGetY(center);
        float cz = dx::XMVectorGetZ(center);
        return dx::XMVectorSet(
            Clamp(px, cx - hx, cx + hx),
            Clamp(py, cy - hy, cy + hy),
            Clamp(pz, cz - hz, cz + hz),
            0.0f);
    }

    /** @brief Closest point on OBB to point P (box local clamp). */
    static dx::XMVECTOR ClosestPointOnOBB(dx::FXMVECTOR p, const BoxCollider& b)
    {
        dx::XMVECTOR c = dx::XMLoadFloat3(&b.center);
        dx::XMVECTOR d = dx::XMVectorSubtract(p, c);
        dx::XMVECTOR ax = dx::XMLoadFloat3(&b.axisX);
        dx::XMVECTOR ay = dx::XMLoadFloat3(&b.axisY);
        dx::XMVECTOR az = dx::XMLoadFloat3(&b.axisZ);
        float lx = Clamp(Dot3(d, ax), -b.half.x, b.half.x);
        float ly = Clamp(Dot3(d, ay), -b.half.y, b.half.y);
        float lz = Clamp(Dot3(d, az), -b.half.z, b.half.z);
        dx::XMVECTOR q = c;
        q = dx::XMVectorAdd(q, dx::XMVectorScale(ax, lx));
        q = dx::XMVectorAdd(q, dx::XMVectorScale(ay, ly));
        q = dx::XMVectorAdd(q, dx::XMVectorScale(az, lz));
        return q;
    }

    bool Intersect(const CapsuleCollider& a, const CapsuleCollider& b)
    {
        dx::XMVECTOR a0 = dx::XMLoadFloat3(&a.pointA);
        dx::XMVECTOR a1 = dx::XMLoadFloat3(&a.pointB);
        dx::XMVECTOR b0 = dx::XMLoadFloat3(&b.pointA);
        dx::XMVECTOR b1 = dx::XMLoadFloat3(&b.pointB);
        dx::XMVECTOR p{};
        dx::XMVECTOR q{};
        float distSq = ClosestPointsSegmentSegment(a0, a1, b0, b1, p, q);
        float r = a.radius + b.radius;
        return distSq <= r * r;
    }

    bool Intersect(const CapsuleCollider& c, const SphereCollider& s)
    {
        dx::XMVECTOR a = dx::XMLoadFloat3(&c.pointA);
        dx::XMVECTOR b = dx::XMLoadFloat3(&c.pointB);
        dx::XMVECTOR sc = dx::XMLoadFloat3(&s.center);
        dx::XMVECTOR closest = ClosestPointOnSegment(a, b, sc);
        float r = c.radius + s.radius;
        return LengthSq3(dx::XMVectorSubtract(sc, closest)) <= r * r;
    }
    bool Intersect(const SphereCollider& s, const CapsuleCollider& c)
    {
        return Intersect(c, s);
    }

    bool Intersect(const CapsuleCollider& c, const PointCollider& p)
    {
        dx::XMVECTOR a = dx::XMLoadFloat3(&c.pointA);
        dx::XMVECTOR b = dx::XMLoadFloat3(&c.pointB);
        dx::XMVECTOR pt = dx::XMLoadFloat3(&p.position);
        dx::XMVECTOR closest = ClosestPointOnSegment(a, b, pt);
        return LengthSq3(dx::XMVectorSubtract(pt, closest)) <= c.radius * c.radius;
    }
    bool Intersect(const PointCollider& p, const CapsuleCollider& c)
    {
        return Intersect(c, p);
    }

    bool Intersect(const CapsuleCollider& cap, const BoxCollider& box)
    {
        // Sample closest approach: closest point on capsule segment to box surface (via closest point on box to each endpoint + segment mid heuristic).
        // Robust approach: treat as sphere of radius r whose center is constrained to the segment —
        // find point on segment minimizing distance to box (iterative / analytic for AABB).
        dx::XMVECTOR a = dx::XMLoadFloat3(&cap.pointA);
        dx::XMVECTOR b = dx::XMLoadFloat3(&cap.pointB);
        dx::XMVECTOR c = dx::XMLoadFloat3(&box.center);

        // For AABB: closest point on segment to AABB = ClosestPointOnSegment(a,b, ClosestPointOnAABB(segmentPoint...))
        // Use: q = ClosestPointOnBox(a); then clamp segment; also q from b; take min distance among samples + analytic for AABB.
        auto distSqSegmentToBox = [&](dx::FXMVECTOR pSeg) -> float
        {
            dx::XMVECTOR q;
            if (box.aabbKind != BoxAABBKind::OBB)
            {
                q = ClosestPointOnAABB(pSeg, c, box.half.x, box.half.y, box.half.z);
            }
            else
            {
                q = ClosestPointOnOBB(pSeg, box);
            }
            return LengthSq3(dx::XMVectorSubtract(pSeg, q));
        };

        // Closest point on segment to box: for AABB, project closest-on-AABB of endpoints and of the point on segment nearest box center.
        dx::XMVECTOR nearestOnSeg = ClosestPointOnSegment(a, b, c);
        float best = distSqSegmentToBox(nearestOnSeg);
        best = (std::min)(best, distSqSegmentToBox(a));
        best = (std::min)(best, distSqSegmentToBox(b));

        // Extra samples along segment for OBB robustness
        for (int i = 1; i <= 4; ++i)
        {
            float t = static_cast<float>(i) / 5.0f;
            dx::XMVECTOR p = dx::XMVectorAdd(a, dx::XMVectorScale(dx::XMVectorSubtract(b, a), t));
            best = (std::min)(best, distSqSegmentToBox(p));
        }

        // Also: closest point on box to segment endpoints pulled back onto segment
        {
            dx::XMVECTOR qa = (box.aabbKind != BoxAABBKind::OBB)
                ? ClosestPointOnAABB(a, c, box.half.x, box.half.y, box.half.z)
                : ClosestPointOnOBB(a, box);
            dx::XMVECTOR qb = (box.aabbKind != BoxAABBKind::OBB)
                ? ClosestPointOnAABB(b, c, box.half.x, box.half.y, box.half.z)
                : ClosestPointOnOBB(b, box);
            dx::XMVECTOR pa = ClosestPointOnSegment(a, b, qa);
            dx::XMVECTOR pb = ClosestPointOnSegment(a, b, qb);
            best = (std::min)(best, distSqSegmentToBox(pa));
            best = (std::min)(best, distSqSegmentToBox(pb));
        }

        return best <= cap.radius * cap.radius;
    }
    bool Intersect(const BoxCollider& b, const CapsuleCollider& c)
    {
        return Intersect(c, b);
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

            set(CollideType::Capsule, CollideType::Capsule, &Dispatch<CapsuleCollider, CapsuleCollider>);
            set(CollideType::Capsule, CollideType::Sphere,  &Dispatch<CapsuleCollider, SphereCollider>, &Dispatch<SphereCollider, CapsuleCollider>);
            set(CollideType::Capsule, CollideType::Point,   &Dispatch<CapsuleCollider, PointCollider>,  &Dispatch<PointCollider, CapsuleCollider>);
            set(CollideType::Capsule, CollideType::Box,     &Dispatch<CapsuleCollider, BoxCollider>,    &Dispatch<BoxCollider, CapsuleCollider>);

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