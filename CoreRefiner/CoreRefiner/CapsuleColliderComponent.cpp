#include "CapsuleColliderComponent.h"
#include "ObjectBase.h"
#include "CapsuleWireframe.h"
#include "RenderGraph.h"
#include "Graphics.h"
#include "XMath.h"

#include <algorithm>
#include <cassert>

CapsuleColliderComponent::CapsuleColliderComponent(
	ObjectBase* owner,
	float radius,
	float totalHeight,
	ColliderSyncMode syncMode) noexcept
	:
	ColliderComponentBase(owner, syncMode)
{
	assert(syncMode_ == ColliderSyncMode::FollowCenter);
	SetCapsule(radius, totalHeight);
}

CapsuleColliderComponent::~CapsuleColliderComponent() = default;

void CapsuleColliderComponent::SyncCapsuleYUp(DirectX::XMFLOAT3 center) noexcept
{
	const float r = (std::max)(capsule_.radius, 0.0f);
	const float h = (std::max)(totalHeight_, 2.0f * r);
	const float halfSeg = (h * 0.5f) - r;
	const Vec3 c{ center };
	const Vec3 yOff{ 0.0f, halfSeg, 0.0f };
	capsule_.pointA = (c + yOff).ToFloat3();
	capsule_.pointB = (c - yOff).ToFloat3();
	capsule_.radius = r;
}

void CapsuleColliderComponent::Submit()
{
#ifdef _DEBUG
	if (!debugDraw_ || debugCapsuleWire_ == nullptr)
	{
		return;
	}
	debugCapsuleWire_->DoSubmit(capsule_.pointA, capsule_.pointB, capsule_.radius);
#endif
}

void CapsuleColliderComponent::SyncFromOwner()
{
	ObjectBase* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}
	SyncCapsuleYUp(ResolveSyncCenter(*owner));
}

Collider3D::Collision3D& CapsuleColliderComponent::GetVolume()
{
	return capsule_;
}

const Collider3D::Collision3D& CapsuleColliderComponent::GetVolume() const
{
	return capsule_;
}

void CapsuleColliderComponent::SetCapsule(float radius, float totalHeight) noexcept
{
	capsule_.radius = (std::max)(radius, 0.0f);
	totalHeight_ = (std::max)(totalHeight, 2.0f * capsule_.radius);
	if (ObjectBase* owner = GetOwner())
	{
		SyncCapsuleYUp(ResolveSyncCenter(*owner));
	}
}

void CapsuleColliderComponent::SetCollisionSize(DirectX::XMFLOAT3 size) noexcept
{
	SetCapsule((std::max)(size.x, size.z) * 0.5f, size.y);
}

void CapsuleColliderComponent::LinkDebugWire(
	Graphics& gfx,
	Rgph::RenderGraph& rg,
	DirectX::XMFLOAT3 color,
	const char* name)
{
#ifdef _DEBUG
	const char* wireName = name != nullptr ? name : "wireCapsuleCollider";
	debugCapsuleWire_ = std::make_unique<CapsuleWireframe>(gfx, color, wireName);
	debugCapsuleWire_->LinkTechniques(rg);
#else
	(void)gfx;
	(void)rg;
	(void)color;
	(void)name;
#endif
}
