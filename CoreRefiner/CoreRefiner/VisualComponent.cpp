#include "VisualComponent.h"
#include "ObjectBase.h"
#include "Drawable.h"

VisualComponent::VisualComponent(
	ObjectBase* owner,
	Drawable* drawable,
	std::size_t channelMask,
	bool yawOnly,
	bool syncScale) noexcept
	:
	IComponent(owner),
	drawable_(drawable),
	channelMask_(channelMask),
	yawOnly_(yawOnly),
	syncScale_(syncScale)
{}

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
	drawable_->SetPosition(owner->GetPosition());

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
}
