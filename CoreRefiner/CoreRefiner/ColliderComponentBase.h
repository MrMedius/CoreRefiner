#pragma once
#include "IComponent.h"
#include "Collision3D.h"
#include <DirectXMath.h>
#include <cstdint>

class Graphics;
namespace Rgph { class RenderGraph; }

// How a collider volume tracks the host transform.
enum class ColliderSyncMode : uint8_t
{
	// Volume center follows host position (+ center offset).
	FollowCenter = 0,
	// Box only: center follows position; axisY stores raw Euler (Enemy_T legacy).
	FollowCenterAxisYFromRotation,
	// Box only: rebuild via BoxCollider::BuildFromWorldMatrix each sync (Field).
	FromWorldMatrix,
};

// Thin polymorphic collider contract (Unity-style base).
// Prefer GetComponent<ColliderComponentBase>() for shape-agnostic queries.
// Shape-specific components (Box / Sphere / Capsule) derive from this.
class ColliderComponentBase : public IComponent
{
public:
	ColliderComponentBase(
		ObjectBase* owner,
		ColliderSyncMode syncMode = ColliderSyncMode::FollowCenter) noexcept;
	~ColliderComponentBase() override;

	void OnEnable() override;
	// No-op: volume + debug wire stay owned for pool reuse.
	void OnDisable() override {}
	void Update(float dt) override;
	void Submit() override = 0;

	// Refresh the registered volume from the host transform.
	virtual void SyncFromOwner() = 0;

	// Polymorphic volume for IsOverlap / TrySeparate.
	[[nodiscard]] virtual Collider3D::Collision3D& GetVolume() = 0;
	[[nodiscard]] virtual const Collider3D::Collision3D& GetVolume() const = 0;

	[[nodiscard]] virtual Collider3D::CollideType GetCollideType() const noexcept = 0;

	// Create and link a debug wireframe for this volume.
	virtual void LinkDebugWire(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 color,
		const char* name = "wireCollider") = 0;

	// Box half extents when type is Box; otherwise {0,0,0}.
	// Box AABB helpers only.
	[[nodiscard]] virtual DirectX::XMFLOAT3 GetCollisionSize() const noexcept
	{
		return {};
	}

	[[nodiscard]] ColliderSyncMode GetSyncMode() const noexcept { return syncMode_; }

	void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }
	[[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

	// Offset added to GetWorldPosition() when syncing the volume center.
	void SetCenterOffset(DirectX::XMFLOAT3 offset) noexcept;
	[[nodiscard]] DirectX::XMFLOAT3 GetCenterOffset() const noexcept { return centerOffset_; }

	void SetDebugDraw(bool enabled) noexcept { debugDraw_ = enabled; }
	[[nodiscard]] bool GetDebugDraw() const noexcept { return debugDraw_; }

protected:
	// GetWorldPosition() + centerOffset_.
	[[nodiscard]] DirectX::XMFLOAT3 ResolveSyncCenter(const ObjectBase& owner) const noexcept;

	ColliderSyncMode syncMode_{ ColliderSyncMode::FollowCenter };
	DirectX::XMFLOAT3 centerOffset_{ 0.0f, 0.0f, 0.0f };
	bool enabled_{ true };
	bool debugDraw_{ true };
};
