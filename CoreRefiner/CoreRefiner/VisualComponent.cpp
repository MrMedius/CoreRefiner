#include "VisualComponent.h"
#include "ObjectBase.h"

#include <algorithm>
#include <cmath>

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
		// Root: legacy Raw Euler → Drawable DegreeToRad (Enemy yawOnly etc.).
		const auto rot = owner->GetRotation();
		if (yawOnly_)
		{
			drawable_->SetRotation(0.0f, rot.y, 0.0f);
		}
		else
		{
			drawable_->SetRotation(rot.x, rot.y, rot.z);
		}

		if (syncScale_)
		{
			drawable_->SetScale(owner->GetSize());
		}
		return;
	}

	// Parented: inherit orientation/scale from world matrix (decompose).
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
	/**
	 * @brief Extract roll/pitch/yaw (radians) matching XMMatrixRotationRollPitchYaw convention.
	 * @note Pitch from -m12; yaw/roll from remaining elements (DirectXMath common pattern).
	 */
	const float r12 = XMVectorGetY(rotM.r[2]);
	const float r00 = XMVectorGetX(rotM.r[0]);
	const float r01 = XMVectorGetY(rotM.r[0]);
	const float r02 = XMVectorGetZ(rotM.r[0]);
	const float r10 = XMVectorGetX(rotM.r[1]);
	const float r11 = XMVectorGetY(rotM.r[1]);
	const float r20 = XMVectorGetX(rotM.r[2]);
	const float r21 = XMVectorGetY(rotM.r[2]);
	const float r22 = XMVectorGetZ(rotM.r[2]);
	(void)r01;
	(void)r10;
	(void)r21;

	float pitch = std::asin((std::max)(-1.0f, (std::min)(1.0f, -r12)));
	float yaw = std::atan2(r02, r22);
	float roll = std::atan2(r10, r11);
	if (std::fabs(r12) > 0.999f)
	{
		yaw = std::atan2(-r20, r00);
		roll = 0.0f;
	}
	(void)r11;

	constexpr float kRadToDeg = 180.0f / 3.14159265f;
	if (yawOnly_)
	{
		drawable_->SetRotation(0.0f, yaw * kRadToDeg, 0.0f);
	}
	else
	{
		drawable_->SetRotation(roll * kRadToDeg, pitch * kRadToDeg, yaw * kRadToDeg);
	}
}
