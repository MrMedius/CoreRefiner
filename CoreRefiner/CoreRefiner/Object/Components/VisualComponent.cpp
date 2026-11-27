#include "VisualComponent.h"
#include "ObjectBase.h"
#include "Framework/Helpers/Mathematics/XMath.h"

VisualComponent::VisualComponent(
	ObjectBase* owner,
	std::unique_ptr<Drawable> drawable,
	std::size_t channelMask,
	bool yawOnly,
	bool syncScale) noexcept
	:
	IComponent(owner),
	drawable_(std::move(drawable)),
	channelMask_(channelMask),
	yawOnly_(yawOnly),
	syncScale_(syncScale)
{
	assert(drawable_ != nullptr && "VisualComponent requires a Drawable");
}

VisualComponent::~VisualComponent() = default;

void VisualComponent::OnEnable()
{
	SyncFromOwner();
}

void VisualComponent::Update(float dt)
{
	(void)dt;
	SyncFromOwner();
}

void VisualComponent::Submit()
{
	if (drawable_ != nullptr)
	{
		drawable_->Submit(channelMask_);
	}
}

void VisualComponent::SyncFromOwner()
{
	if (drawable_ == nullptr || GetOwner() == nullptr)
	{
		return;
	}

	ObjectBase* owner = GetOwner();
	// World translation (≡ local when unparented) — required for parented visuals.
	drawable_->SetPosition(owner->GetWorldPosition());

	if (owner->GetParent() == nullptr)
	{
		// 物体旋转已是弧度。根物体按本地欧拉角直接写入外形。
		const auto rot = owner->GetRotation();
		if (yawOnly_)
		{
			drawable_->SetRotationRad(0.0f, rot.y, 0.0f);
		}
		else
		{
			drawable_->SetRotationRad(rot.x, rot.y, rot.z);
		}

		if (syncScale_)
		{
			drawable_->SetScale(owner->GetSize());
		}
		return;
	}

	// 挂在父物体上时，朝向和缩放都从世界矩阵拆出来。
	using namespace DirectX;
	XMVECTOR scaleV{};
	XMVECTOR quatV{};
	XMVECTOR transV{};
	if (!XMMatrixDecompose(&scaleV, &quatV, &transV, owner->GetWorldMatrix()))
	{
		if (syncScale_)
		{
			drawable_->SetScale(owner->GetSize());
		}
		return;
	}

	if (syncScale_)
	{
		XMFLOAT3 scale{};
		XMStoreFloat3(&scale, scaleV);
		drawable_->SetScale(scale);
	}

	const XMMATRIX rotM = XMMatrixRotationQuaternion(quatV);
	// euler 是世界旋转拆出的弧度。x 是 pitch，y 是 yaw，z 是 roll。
	XMFLOAT4X4 rot4{};
	XMStoreFloat4x4(&rot4, rotM);
	const XMFLOAT3 euler = ExtractEulerAngles(rot4);
	if (yawOnly_)
	{
		drawable_->SetRotationRad(0.0f, euler.y, 0.0f);
	}
	else
	{
		drawable_->SetRotationRad(euler.x, euler.y, euler.z);
	}
}
