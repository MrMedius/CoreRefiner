#include "SphereColliderComponent.h"
#include "ObjectBase.h"
#include "SphereWireframe.h"
#include "RenderGraph.h"
#include "Graphics.h"

#include <algorithm>
#include <cassert>

SphereColliderComponent::SphereColliderComponent(
	ObjectBase* owner,
	float radius,
	ColliderSyncMode syncMode) noexcept
	:
	ColliderComponentBase(owner, syncMode)
{
	assert(syncMode_ == ColliderSyncMode::FollowCenter);
	sphere_.radius = (std::max)(radius, 0.0f);
}

SphereColliderComponent::~SphereColliderComponent() = default;

void SphereColliderComponent::Submit()
{
#ifdef _DEBUG
	if (!debugDraw_ || debugSphereWire_ == nullptr)
	{
		return;
	}
	debugSphereWire_->DoSubmit(sphere_.center, sphere_.radius * 2.0f);
#endif
}

void SphereColliderComponent::SyncFromOwner()
{
	ObjectBase* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}
	sphere_.center = ResolveSyncCenter(*owner);
}

Collider3D::Collision3D& SphereColliderComponent::GetVolume()
{
	return sphere_;
}

const Collider3D::Collision3D& SphereColliderComponent::GetVolume() const
{
	return sphere_;
}

void SphereColliderComponent::SetRadius(float radius) noexcept
{
	sphere_.radius = (std::max)(radius, 0.0f);
}

void SphereColliderComponent::SetCollisionSize(DirectX::XMFLOAT3 size) noexcept
{
	const float m = (std::max)(size.x, (std::max)(size.y, size.z));
	SetRadius(m * 0.5f);
}

void SphereColliderComponent::LinkDebugWire(
	Graphics& gfx,
	Rgph::RenderGraph& rg,
	DirectX::XMFLOAT3 color,
	const char* name)
{
#ifdef _DEBUG
	const char* wireName = name != nullptr ? name : "wireSphereCollider";
	debugSphereWire_ = std::make_unique<SphereWireframe>(gfx, color, wireName);
	debugSphereWire_->LinkTechniques(rg);
#else
	(void)gfx;
	(void)rg;
	(void)color;
	(void)name;
#endif
}
