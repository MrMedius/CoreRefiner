#pragma once
#include "ColliderComponentBase.h"
#include <memory>

#ifdef _DEBUG
#include "CapsuleWireframe.h"
#endif

// Capsule-only collider component (Unity-style CapsuleCollider).
// Axis is world Y; endpoints rebuilt each sync from center, radius, totalHeight.
class CapsuleColliderComponent : public ColliderComponentBase
{
public:
	// owner Host entity (injected by AddComponent).
	// radius Capsule radius.
	// totalHeight Full height including both hemispheres.
	// syncMode Must be FollowCenter.
	CapsuleColliderComponent(
		ObjectBase* owner,
		float radius = 0.5f,
		float totalHeight = 2.0f,
		ColliderSyncMode syncMode = ColliderSyncMode::FollowCenter) noexcept;
	~CapsuleColliderComponent() override;

	void Submit() override;
	void SyncFromOwner() override;

	[[nodiscard]] Collider3D::Collision3D& GetVolume() override;
	[[nodiscard]] const Collider3D::Collision3D& GetVolume() const override;
	[[nodiscard]] Collider3D::CollideType GetCollideType() const noexcept override
	{
		return Collider3D::CollideType::Capsule;
	}

	void LinkDebugWire(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 color,
		const char* name = "wireCollider") override;

	// Set radius + total height (axis world Y).
	// totalHeight includes both hemispheres; segment = max(0, totalHeight - 2*radius).
	void SetCapsule(float radius, float totalHeight) noexcept;
	[[nodiscard]] float GetRadius() const noexcept { return capsule_.radius; }
	[[nodiscard]] float GetTotalHeight() const noexcept { return totalHeight_; }

	// Compatibility helper: diameter = max(x,z), totalHeight = y (legacy SetCollisionSize).
	void SetCollisionSize(DirectX::XMFLOAT3 size) noexcept;

	[[nodiscard]] Collider3D::CapsuleCollider* TryGetCapsule() noexcept { return &capsule_; }
	[[nodiscard]] const Collider3D::CapsuleCollider* TryGetCapsule() const noexcept { return &capsule_; }

private:
	// Rebuild Y-up capsule endpoints from center, radius, and total height.
	void SyncCapsuleYUp(DirectX::XMFLOAT3 center) noexcept;

	Collider3D::CapsuleCollider capsule_{};
	float totalHeight_{ 2.0f };

#ifdef _DEBUG
	std::unique_ptr<CapsuleWireframe> debugCapsuleWire_;
#endif
};
