#pragma once
#include "ColliderComponentBase.h"
#include <memory>

#ifdef _DEBUG
#include "SphereWireframe.h"
#endif

/**
 * @brief Sphere-only collider component (Unity-style SphereCollider).
 * @note Sync mode is FollowCenter only (center = host position + center offset).
 */
class SphereColliderComponent : public ColliderComponentBase
{
public:
	/**
	 * @param owner Host entity (injected by AddComponent).
	 * @param radius Sphere radius in world units.
	 * @param syncMode Must be FollowCenter.
	 */
	SphereColliderComponent(
		ObjectBase* owner,
		float radius = 1.0f,
		ColliderSyncMode syncMode = ColliderSyncMode::FollowCenter) noexcept;
	~SphereColliderComponent() override;

	void Submit() override;
	void SyncFromOwner() override;

	[[nodiscard]] Collider3D::Collision3D& GetVolume() override;
	[[nodiscard]] const Collider3D::Collision3D& GetVolume() const override;
	[[nodiscard]] Collider3D::CollideType GetCollideType() const noexcept override
	{
		return Collider3D::CollideType::Sphere;
	}

	void LinkDebugWire(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 color,
		const char* name = "wireCollider") override;

	/**
	 * @brief Set radius directly.
	 */
	void SetRadius(float radius) noexcept;
	[[nodiscard]] float GetRadius() const noexcept { return sphere_.radius; }

	/**
	 * @brief Compatibility helper: radius = max(size) * 0.5f (legacy SetCollisionSize).
	 */
	void SetCollisionSize(DirectX::XMFLOAT3 size) noexcept;

	[[nodiscard]] Collider3D::SphereCollider* TryGetSphere() noexcept { return &sphere_; }
	[[nodiscard]] const Collider3D::SphereCollider* TryGetSphere() const noexcept { return &sphere_; }

private:
	Collider3D::SphereCollider sphere_{};

#ifdef _DEBUG
	std::unique_ptr<SphereWireframe> debugSphereWire_;
#endif
};
