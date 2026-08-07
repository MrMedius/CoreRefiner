#include "ColliderComponentBase.h"
#include "ObjectBase.h"
#include "XMath.h"

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
	return (V(owner.GetWorldPosition()) + V(centerOffset_)).ToFloat3();
}

void ColliderComponentBase::SetCenterOffset(DirectX::XMFLOAT3 offset) noexcept
{
	centerOffset_ = offset;
	SyncFromOwner();
}
