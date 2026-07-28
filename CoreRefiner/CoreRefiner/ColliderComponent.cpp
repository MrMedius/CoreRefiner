#include "ColliderComponent.h"
#include "ObjectBase.h"
#include "CubeWireframe.h"
#include "SphereWireframe.h"
#include "CapsuleWireframe.h"
#include "RenderGraph.h"
#include "Graphics.h"

#include <algorithm>
#include <cassert>

namespace
{
	float Max3(DirectX::XMFLOAT3 v) noexcept
	{
		return (std::max)(v.x, (std::max)(v.y, v.z));
	}

	/**
	 * @brief Rebuild Y-up capsule endpoints from center, radius, and total height.
	 */
	void SyncCapsuleYUp(Collider3D::CapsuleCollider& cap, DirectX::XMFLOAT3 center, float totalHeight) noexcept
	{
		const float r = (std::max)(cap.radius, 0.0f);
		const float h = (std::max)(totalHeight, 2.0f * r);
		const float halfSeg = (h * 0.5f) - r;
		cap.pointA = { center.x, center.y + halfSeg, center.z };
		cap.pointB = { center.x, center.y - halfSeg, center.z };
		cap.radius = r;
	}

	DirectX::XMFLOAT3 Add3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b) noexcept
	{
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}
}

DirectX::XMFLOAT3 ColliderComponent::ResolveSyncCenter(const ObjectBase& owner) const noexcept
{
	return Add3(owner.GetPosition(), centerOffset_);
}

ColliderComponent::ColliderComponent(
	ObjectBase* owner,
	Collider3D::CollideType type,
	ColliderSyncMode syncMode,
	DirectX::XMFLOAT3 worldMatrixLocalHalf) noexcept
	:
	IComponent(owner),
	type_(type),
	syncMode_(syncMode),
	worldMatrixLocalHalf_(worldMatrixLocalHalf)
{
	assert(type != Collider3D::CollideType::Count);
	if (syncMode_ == ColliderSyncMode::FollowCenterAxisYFromRotation ||
		syncMode_ == ColliderSyncMode::FromWorldMatrix)
	{
		assert(type_ == Collider3D::CollideType::Box);
	}
	RegisterVolume(owner);
}

ColliderComponent::~ColliderComponent() = default;

void ColliderComponent::RegisterVolume(ObjectBase* owner)
{
	(void)owner;
	switch (type_)
	{
	case Collider3D::CollideType::Box:
		volume_ = Collider3D::BoxCollider{};
		break;
	case Collider3D::CollideType::Sphere:
		volume_ = Collider3D::SphereCollider{};
		break;
	case Collider3D::CollideType::Point:
		volume_ = Collider3D::PointCollider{};
		break;
	case Collider3D::CollideType::Capsule:
		volume_ = Collider3D::CapsuleCollider{};
		break;
	default:
		volume_ = Collider3D::BoxCollider{};
		type_ = Collider3D::CollideType::Box;
		break;
	}
}

void ColliderComponent::OnEnable()
{
	SyncFromOwner();
}

void ColliderComponent::Update(float dt)
{
	(void)dt;
	SyncFromOwner();
}

void ColliderComponent::Submit()
{
#ifdef _DEBUG
	if (!debugDraw_)
	{
		return;
	}
	ObjectBase* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}

	if (const Collider3D::BoxCollider* box = TryGetBox())
	{
		if (debugBoxWire_ == nullptr)
		{
			return;
		}
		const DirectX::XMFLOAT3 size{ box->half.x * 2.0f, box->half.y * 2.0f, box->half.z * 2.0f };
		if (syncMode_ == ColliderSyncMode::FollowCenterAxisYFromRotation)
		{
			debugBoxWire_->DoSubmit(box->center, owner->GetRotation(), size);
		}
		else
		{
			debugBoxWire_->DoSubmit(box->center, size);
		}
		return;
	}

	if (const Collider3D::SphereCollider* sphere = TryGetSphere())
	{
		if (debugSphereWire_ == nullptr)
		{
			return;
		}
		debugSphereWire_->DoSubmit(sphere->center, sphere->radius * 2.0f);
		return;
	}

	if (const Collider3D::CapsuleCollider* capsule = TryGetCapsule())
	{
		if (debugCapsuleWire_ == nullptr)
		{
			return;
		}
		debugCapsuleWire_->DoSubmit(capsule->pointA, capsule->pointB, capsule->radius);
	}
#endif
}

void ColliderComponent::SyncFromOwner()
{
	ObjectBase* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT3 syncCenter = ResolveSyncCenter(*owner);

	switch (type_)
	{
	case Collider3D::CollideType::Box:
	{
		auto& box = std::get<Collider3D::BoxCollider>(volume_);
		switch (syncMode_)
		{
		case ColliderSyncMode::FollowCenter:
			box.center = syncCenter;
			break;
		case ColliderSyncMode::FollowCenterAxisYFromRotation:
			box.center = syncCenter;
			box.axisY = owner->GetRotation();
			break;
		case ColliderSyncMode::FromWorldMatrix:
			box = Collider3D::BoxCollider::BuildFromWorldMatrix(
				owner->GetTransform().GetInfo().GetWorldMatrix(),
				worldMatrixLocalHalf_);
			box.center = Add3(box.center, centerOffset_);
			break;
		}
		break;
	}
	case Collider3D::CollideType::Sphere:
	{
		auto& sphere = std::get<Collider3D::SphereCollider>(volume_);
		sphere.center = syncCenter;
		break;
	}
	case Collider3D::CollideType::Point:
	{
		auto& point = std::get<Collider3D::PointCollider>(volume_);
		point.position = syncCenter;
		break;
	}
	case Collider3D::CollideType::Capsule:
	{
		auto& capsule = std::get<Collider3D::CapsuleCollider>(volume_);
		SyncCapsuleYUp(capsule, syncCenter, capsuleTotalHeight_);
		break;
	}
	default:
		break;
	}
}

Collider3D::Collision3D& ColliderComponent::GetVolume()
{
	return std::visit(
		[](auto& v) -> Collider3D::Collision3D& { return v; },
		volume_);
}

const Collider3D::Collision3D& ColliderComponent::GetVolume() const
{
	return std::visit(
		[](const auto& v) -> const Collider3D::Collision3D& { return v; },
		volume_);
}

Collider3D::BoxCollider* ColliderComponent::TryGetBox() noexcept
{
	return std::get_if<Collider3D::BoxCollider>(&volume_);
}

const Collider3D::BoxCollider* ColliderComponent::TryGetBox() const noexcept
{
	return std::get_if<Collider3D::BoxCollider>(&volume_);
}

Collider3D::SphereCollider* ColliderComponent::TryGetSphere() noexcept
{
	return std::get_if<Collider3D::SphereCollider>(&volume_);
}

const Collider3D::SphereCollider* ColliderComponent::TryGetSphere() const noexcept
{
	return std::get_if<Collider3D::SphereCollider>(&volume_);
}

Collider3D::CapsuleCollider* ColliderComponent::TryGetCapsule() noexcept
{
	return std::get_if<Collider3D::CapsuleCollider>(&volume_);
}

const Collider3D::CapsuleCollider* ColliderComponent::TryGetCapsule() const noexcept
{
	return std::get_if<Collider3D::CapsuleCollider>(&volume_);
}

Collider3D::BoxCollider ColliderComponent::GetBoxCollider() const
{
	const Collider3D::BoxCollider* box = TryGetBox();
	assert(box != nullptr && "GetBoxCollider requires Box type");
	if (box != nullptr)
	{
		return *box;
	}
	return {};
}

DirectX::XMFLOAT3 ColliderComponent::GetCollisionSize() const noexcept
{
	if (const Collider3D::BoxCollider* box = TryGetBox())
	{
		return box->half;
	}
	return {};
}

void ColliderComponent::SetCollisionSize(DirectX::XMFLOAT3 size) noexcept
{
	switch (type_)
	{
	case Collider3D::CollideType::Box:
		if (auto* box = TryGetBox())
		{
			box->half = { size.x / 2.0f, size.y / 2.0f, size.z / 2.0f };
		}
		break;
	case Collider3D::CollideType::Sphere:
		if (auto* sphere = TryGetSphere())
		{
			sphere->radius = Max3(size) * 0.5f;
		}
		break;
	case Collider3D::CollideType::Capsule:
		SetCapsule((std::max)(size.x, size.z) * 0.5f, size.y);
		break;
	case Collider3D::CollideType::Point:
	default:
		break;
	}
}

void ColliderComponent::SetSphereRadius(float radius) noexcept
{
	if (auto* sphere = TryGetSphere())
	{
		sphere->radius = radius;
	}
}

void ColliderComponent::SetCapsule(float radius, float totalHeight) noexcept
{
	if (auto* capsule = TryGetCapsule())
	{
		capsule->radius = (std::max)(radius, 0.0f);
		capsuleTotalHeight_ = (std::max)(totalHeight, 2.0f * capsule->radius);
		if (ObjectBase* owner = GetOwner())
		{
			SyncCapsuleYUp(*capsule, ResolveSyncCenter(*owner), capsuleTotalHeight_);
		}
	}
}

void ColliderComponent::SetCenterOffset(DirectX::XMFLOAT3 offset) noexcept
{
	centerOffset_ = offset;
	SyncFromOwner();
}

void ColliderComponent::LinkDebugWire(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 color, const char* name)
{
#ifdef _DEBUG
	const char* wireName = name != nullptr ? name : "wireCollider";
	debugBoxWire_.reset();
	debugSphereWire_.reset();
	debugCapsuleWire_.reset();
	switch (type_)
	{
	case Collider3D::CollideType::Box:
		debugBoxWire_ = std::make_unique<CubeWireframe>(gfx, color, wireName);
		debugBoxWire_->LinkTechniques(rg);
		break;
	case Collider3D::CollideType::Sphere:
		debugSphereWire_ = std::make_unique<SphereWireframe>(gfx, color, wireName);
		debugSphereWire_->LinkTechniques(rg);
		break;
	case Collider3D::CollideType::Capsule:
		debugCapsuleWire_ = std::make_unique<CapsuleWireframe>(gfx, color, wireName);
		debugCapsuleWire_->LinkTechniques(rg);
		break;
	case Collider3D::CollideType::Point:
	default:
		break;
	}
#else
	(void)gfx;
	(void)rg;
	(void)color;
	(void)name;
#endif
}
