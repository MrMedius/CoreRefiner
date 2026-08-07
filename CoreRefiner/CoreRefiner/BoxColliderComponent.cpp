#include "BoxColliderComponent.h"
#include "ObjectBase.h"
#include "CubeWireframe.h"
#include "RenderGraph.h"
#include "Graphics.h"
#include "XMath.h"

#include <cassert>

BoxColliderComponent::BoxColliderComponent(
	ObjectBase* owner,
	ColliderSyncMode syncMode,
	DirectX::XMFLOAT3 worldMatrixLocalHalf) noexcept
	:
	ColliderComponentBase(owner, syncMode),
	worldMatrixLocalHalf_(worldMatrixLocalHalf)
{
	assert(
		syncMode_ == ColliderSyncMode::FollowCenter ||
		syncMode_ == ColliderSyncMode::FollowCenterAxisYFromRotation ||
		syncMode_ == ColliderSyncMode::FromWorldMatrix);
}

BoxColliderComponent::~BoxColliderComponent() = default;

void BoxColliderComponent::Submit()
{
#ifdef _DEBUG
	if (!debugDraw_ || debugBoxWire_ == nullptr)
	{
		return;
	}
	ObjectBase* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT3 size = (V(box_.half) * 2.0f).ToFloat3();
	if (syncMode_ == ColliderSyncMode::FollowCenterAxisYFromRotation)
	{
		debugBoxWire_->DoSubmit(box_.center, owner->GetRotation(), size);
	}
	else
	{
		debugBoxWire_->DoSubmit(box_.center, size);
	}
#endif
}

void BoxColliderComponent::SyncFromOwner()
{
	ObjectBase* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT3 syncCenter = ResolveSyncCenter(*owner);
	switch (syncMode_)
	{
	case ColliderSyncMode::FollowCenter:
		box_.center = syncCenter;
		break;
	case ColliderSyncMode::FollowCenterAxisYFromRotation:
		box_.center = syncCenter;
		box_.axisY = owner->GetRotation();
		break;
	case ColliderSyncMode::FromWorldMatrix:
	{
		DirectX::XMFLOAT4X4 world{};
		DirectX::XMStoreFloat4x4(&world, owner->GetWorldMatrix());
		box_ = Collider3D::BoxCollider::BuildFromWorldMatrix(
			world,
			worldMatrixLocalHalf_);
		box_.center = (V(box_.center) + V(centerOffset_)).ToFloat3();
		break;
	}
	}
}

Collider3D::Collision3D& BoxColliderComponent::GetVolume()
{
	return box_;
}

const Collider3D::Collision3D& BoxColliderComponent::GetVolume() const
{
	return box_;
}

DirectX::XMFLOAT3 BoxColliderComponent::GetCollisionSize() const noexcept
{
	return box_.half;
}

void BoxColliderComponent::SetCollisionSize(DirectX::XMFLOAT3 size) noexcept
{
	box_.half = (V(size) * 0.5f).ToFloat3();
}

void BoxColliderComponent::LinkDebugWire(
	Graphics& gfx,
	Rgph::RenderGraph& rg,
	DirectX::XMFLOAT3 color,
	const char* name)
{
#ifdef _DEBUG
	const char* wireName = name != nullptr ? name : "wireBoxCollider";
	debugBoxWire_ = std::make_unique<CubeWireframe>(gfx, color, wireName);
	debugBoxWire_->LinkTechniques(rg);
#else
	(void)gfx;
	(void)rg;
	(void)color;
	(void)name;
#endif
}
