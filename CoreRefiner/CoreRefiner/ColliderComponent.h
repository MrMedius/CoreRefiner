#pragma once
#include "IComponent.h"
#include "Collision.h"
#include <DirectXMath.h>
#include <cstdint>
#include <memory>
#include <variant>

class Graphics;
namespace Rgph { class RenderGraph; }

#ifdef _DEBUG
#include "CubeWireframe.h"
#include "SphereWireframe.h"
#include "CapsuleWireframe.h"
#endif

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
 * @note Owns volume data, enable flag, and optional debug wire.
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
	~ColliderComponent() override;

	void OnEnable() override;
	/**
	 * @brief No-op: volume + debug wire stay owned for pool reuse.
	 */
	void OnDisable() override {}
	void Update(float dt) override;
	void Submit() override;

	/** @brief Refresh the registered volume from the host transform. */
	void SyncFromOwner();

	[[nodiscard]] Collider3D::CollideType GetCollideType() const noexcept { return type_; }
	[[nodiscard]] ColliderSyncMode GetSyncMode() const noexcept { return syncMode_; }

	void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }
	[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

	/** @brief Polymorphic volume for CollisionSystem::IsOverlap. */
	[[nodiscard]] Collider3D::Collision3D& GetVolume();
	[[nodiscard]] const Collider3D::Collision3D& GetVolume() const;

	/** @brief Box accessor; returns nullptr if registered type is not Box. */
	[[nodiscard]] Collider3D::BoxCollider* TryGetBox() noexcept;
	[[nodiscard]] const Collider3D::BoxCollider* TryGetBox() const noexcept;

	/** @brief Sphere accessor; returns nullptr if registered type is not Sphere. */
	[[nodiscard]] Collider3D::SphereCollider* TryGetSphere() noexcept;
	[[nodiscard]] const Collider3D::SphereCollider* TryGetSphere() const noexcept;

	/** @brief Capsule accessor; returns nullptr if registered type is not Capsule. */
	[[nodiscard]] Collider3D::CapsuleCollider* TryGetCapsule() noexcept;
	[[nodiscard]] const Collider3D::CapsuleCollider* TryGetCapsule() const noexcept;

	/**
	 * @brief Copy of box volume; asserts if type is not Box.
	 */
	[[nodiscard]] Collider3D::BoxCollider GetBoxCollider() const;

	/**
	 * @brief Box half extents (or empty if not Box).
	 */
	[[nodiscard]] DirectX::XMFLOAT3 GetCollisionSize() const noexcept;

	/**
	 * @brief Set full size into the registered volume.
	 * @note Box: half = size/2. Sphere: radius = max(size)/2.
	 *       Capsule: diameter = max(x,z), totalHeight = y (axis = world Y).
	 *       Point: no-op.
	 * @warning Do not call under FromWorldMatrix sync (overwrites matrix half).
	 */
	void SetCollisionSize(DirectX::XMFLOAT3 size) noexcept;

	/**
	 * @brief Set sphere radius directly (no-op if type is not Sphere).
	 */
	void SetSphereRadius(float radius) noexcept;

	/**
	 * @brief Set capsule radius + total height (axis world Y; no-op if not Capsule).
	 * @note totalHeight includes both hemispheres; segment = max(0, totalHeight - 2*radius).
	 */
	void SetCapsule(float radius, float totalHeight) noexcept;

	/**
	 * @brief Create and link a debug wireframe matching the registered volume type.
	 * @note Box → CubeWireframe, Sphere → SphereWireframe, Capsule → CapsuleWireframe.
	 */
	void LinkDebugWire(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 color, const char* name = "wireCollider");

	void SetDebugDraw(bool enabled) noexcept { debugDraw_ = enabled; }
	[[nodiscard]] bool GetDebugDraw() const noexcept { return debugDraw_; }

private:
	void RegisterVolume(ObjectBase* owner);

	Collider3D::CollideType type_{ Collider3D::CollideType::Box };
	ColliderSyncMode syncMode_{ ColliderSyncMode::FollowCenter };
	DirectX::XMFLOAT3 worldMatrixLocalHalf_{ 0.5f, 0.5f, 0.5f };
	bool enabled_{ true };
	bool debugDraw_{ true };

	std::variant<
		Collider3D::BoxCollider,
		Collider3D::SphereCollider,
		Collider3D::PointCollider,
		Collider3D::CapsuleCollider> volume_;

	/** @brief Capsule total height (FollowCenter Y-up); used with radius in SyncFromOwner. */
	float capsuleTotalHeight_{ 2.0f };

#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> debugBoxWire_;
	std::unique_ptr<SphereWireframe> debugSphereWire_;
	std::unique_ptr<CapsuleWireframe> debugCapsuleWire_;
#endif
};
