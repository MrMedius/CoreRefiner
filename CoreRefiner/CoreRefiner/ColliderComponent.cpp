#include "ColliderComponent.h"
#include "ObjectBase.h"
#include "CubeWireframe.h"
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
	if (!debugDraw_ || debugWire_ == nullptr)
	{
		return;
	}
	ObjectBase* owner = GetOwner();
	const Collider3D::BoxCollider* box = TryGetBox();
	if (owner == nullptr || box == nullptr)
	{
		return;
	}
	if (syncMode_ == ColliderSyncMode::FollowCenterAxisYFromRotation)
	{
		debugWire_->DoSubmit(
			owner->GetPosition(),
			owner->GetRotation(),
			{ box->half.x * 2.0f, box->half.y * 2.0f, box->half.z * 2.0f });
	}
	else
	{
		debugWire_->DoSubmit(
			owner->GetPosition(),
			{ box->half.x * 2.0f, box->half.y * 2.0f, box->half.z * 2.0f });
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

	switch (type_)
	{
	case Collider3D::CollideType::Box:
	{
		auto& box = std::get<Collider3D::BoxCollider>(volume_);
		switch (syncMode_)
		{
		case ColliderSyncMode::FollowCenter:
			box.center = owner->GetPosition();
			break;
		case ColliderSyncMode::FollowCenterAxisYFromRotation:
			box.center = owner->GetPosition();
			box.axisY = owner->GetRotation();
			break;
		case ColliderSyncMode::FromWorldMatrix:
			box = Collider3D::BoxCollider::BuildFromWorldMatrix(
				owner->GetTransform().GetInfo().GetWorldMatrix(),
				worldMatrixLocalHalf_);
			break;
		}
		break;
	}
	case Collider3D::CollideType::Sphere:
	{
		auto& sphere = std::get<Collider3D::SphereCollider>(volume_);
		sphere.center = owner->GetPosition();
		break;
	}
	case Collider3D::CollideType::Point:
	{
		auto& point = std::get<Collider3D::PointCollider>(volume_);
		point.position = owner->GetPosition();
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
	case Collider3D::CollideType::Point:
	default:
		break;
	}
}

void ColliderComponent::LinkDebugWire(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 color, const char* name)
{
#ifdef _DEBUG
	debugWire_ = std::make_unique<CubeWireframe>(gfx, color, name != nullptr ? name : "wireBox");
	debugWire_->LinkTechniques(rg);
#else
	(void)gfx;
	(void)rg;
	(void)color;
	(void)name;
#endif
}
