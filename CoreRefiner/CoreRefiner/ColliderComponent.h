#pragma once
#include "IComponent.h"
#include "Collision.h"
#include <DirectXMath.h>
#include <cstdint>
#include <variant>

/**
 * @brief How the registered volume tracks the host transform.
 */
enum class ColliderSyncMode : uint8_t
{
	/** @brief box/sphere/point center (or point position) follows host position. */
	FollowCenter = 0,
	/**
	 * @brief Box only: center follows position; axisY stores raw Euler (Enemy_T legacy).
	 */
	FollowCenterAxisYFromRotation,
	/**
	 * @brief Box only: rebuild via BoxCollider::BuildFromWorldMatrix each sync (Field).
	 */
	FromWorldMatrix,
};

/**
 * @brief Host-driven collision volume with a type chosen at construction time.
 * @note Supports Box / Sphere / Point from Collider3D; gameplay currently registers Box.
 */
class ColliderComponent : public IComponent
{
public:
	/**
	 * @param owner Host entity (injected by AddComponent).
	 * @param type Collision volume kind to register.
	 * @param syncMode Transform sync policy for that volume.
	 * @param worldMatrixLocalHalf Used only when syncMode == FromWorldMatrix.
	 */
	ColliderComponent(
		ObjectBase* owner,
		Collider3D::CollideType type,
		ColliderSyncMode syncMode = ColliderSyncMode::FollowCenter,
		DirectX::XMFLOAT3 worldMatrixLocalHalf = { 0.5f, 0.5f, 0.5f }) noexcept;

	void OnEnable() override;
	void Update(float dt) override;

	/** @brief Refresh the registered volume from the host transform. */
	void SyncFromOwner();

	[[nodiscard]] Collider3D::CollideType GetCollideType() const noexcept { return type_; }
	[[nodiscard]] ColliderSyncMode GetSyncMode() const noexcept { return syncMode_; }

	/** @brief Polymorphic volume for CollisionSystem::IsOverlap. */
	[[nodiscard]] Collider3D::Collision3D& GetVolume();
	[[nodiscard]] const Collider3D::Collision3D& GetVolume() const;

	/** @brief Box accessor; returns nullptr if registered type is not Box. */
	[[nodiscard]] Collider3D::BoxCollider* TryGetBox() noexcept;
	[[nodiscard]] const Collider3D::BoxCollider* TryGetBox() const noexcept;

	/** @brief Sphere accessor; returns nullptr if registered type is not Sphere. */
	[[nodiscard]] Collider3D::SphereCollider* TryGetSphere() noexcept;
	[[nodiscard]] const Collider3D::SphereCollider* TryGetSphere() const noexcept;

	/**
	 * @brief Set full size into the registered volume.
	 * @note Box: half = size/2. Sphere: radius = max(size)/2. Point: no-op.
	 */
	void SetCollisionSize(DirectX::XMFLOAT3 size) noexcept;

private:
	void RegisterVolume(ObjectBase* owner);

	Collider3D::CollideType type_{ Collider3D::CollideType::Box };
	ColliderSyncMode syncMode_{ ColliderSyncMode::FollowCenter };
	DirectX::XMFLOAT3 worldMatrixLocalHalf_{ 0.5f, 0.5f, 0.5f };

	std::variant<
		Collider3D::BoxCollider,
		Collider3D::SphereCollider,
		Collider3D::PointCollider> volume_;
};
