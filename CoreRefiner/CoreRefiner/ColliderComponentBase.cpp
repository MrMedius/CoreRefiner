#include "ColliderComponentBase.h"
#include "ObjectBase.h"

namespace
{
	DirectX::XMFLOAT3 Add3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b) noexcept
	{
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}
}

ColliderComponentBase::ColliderComponentBase(
	ObjectBase* owner,
	ColliderSyncMode syncMode) noexcept
	:
	IComponent(owner),
	syncMode_(syncMode)
{
}

ColliderComponentBase::~ColliderComponentBase() = default;

void ColliderComponentBase::OnEnable()
{
	SyncFromOwner();
}

void ColliderComponentBase::Update(float dt)
{
	(void)dt;
	SyncFromOwner();
}

DirectX::XMFLOAT3 ColliderComponentBase::ResolveSyncCenter(const ObjectBase& owner) const noexcept
{
	// World center so parented volumes follow the hierarchy (Local ≡ World when unparented).
	return Add3(owner.GetWorldPosition(), centerOffset_);
}

void ColliderComponentBase::SetCenterOffset(DirectX::XMFLOAT3 offset) noexcept
{
	centerOffset_ = offset;
	SyncFromOwner();
}
