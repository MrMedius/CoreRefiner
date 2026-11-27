#pragma once
#include "ColliderComponentBase.h"
#include <memory>

#ifdef _DEBUG
#include "CubeWireframe.h"
#endif

// Box-only collider component (Unity-style BoxCollider).
// Supports FollowCenter / FollowCenterAxisYFromRotation / FromWorldMatrix sync modes.
class BoxColliderComponent : public ColliderComponentBase
{
public:
	// owner Host entity (injected by AddComponent).
	// syncMode Transform sync policy for the box.
	// worldMatrixLocalHalf Used only when syncMode == FromWorldMatrix.
	BoxColliderComponent(
		ObjectBase* owner,
		ColliderSyncMode syncMode = ColliderSyncMode::FollowCenter,
		DirectX::XMFLOAT3 worldMatrixLocalHalf = { 0.5f, 0.5f, 0.5f }) noexcept;
	~BoxColliderComponent() override;

	void Submit() override;
	void SyncFromOwner() override;

	[[nodiscard]] Collider3D::Collision3D& GetVolume() override;
	[[nodiscard]] const Collider3D::Collision3D& GetVolume() const override;
	[[nodiscard]] Collider3D::CollideType GetCollideType() const noexcept override
	{
		return Collider3D::CollideType::Box;
	}

	void LinkDebugWire(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 color,
		const char* name = "wireCollider") override;

	[[nodiscard]] DirectX::XMFLOAT3 GetCollisionSize() const noexcept override;

	// Set full size; stores half = size/2.
	// Do not call under FromWorldMatrix sync (overwrites matrix half).
	void SetCollisionSize(DirectX::XMFLOAT3 size) noexcept;

	[[nodiscard]] Collider3D::BoxCollider* TryGetBox() noexcept { return &box_; }
	[[nodiscard]] const Collider3D::BoxCollider* TryGetBox() const noexcept { return &box_; }
	[[nodiscard]] Collider3D::BoxCollider GetBoxCollider() const { return box_; }

private:
	Collider3D::BoxCollider box_{};
	DirectX::XMFLOAT3 worldMatrixLocalHalf_{ 0.5f, 0.5f, 0.5f };

#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> debugBoxWire_;
#endif
};
