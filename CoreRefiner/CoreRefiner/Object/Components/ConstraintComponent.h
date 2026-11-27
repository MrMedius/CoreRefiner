#pragma once
#include "IComponent.h"
#include "ObjectBase.h"

// 约束。只记下要跟着的目标，不拥有，也不改移动。
class ConstraintComponent : public IComponent
{
public:
	// owner 是带着这份约束的物体。target 是目标，可空。
	ConstraintComponent(ObjectBase* owner, ObjectBase* target = nullptr) noexcept
		:
		IComponent(owner),
		target_(target)
	{}

	// 写下要跟着的目标。不拥有。
	void SetTarget(ObjectBase* target) noexcept
	{
		target_ = target;
	}

	// 目标还在场上才返回它。空或已经停用就返回空。
	[[nodiscard]] ObjectBase* GetTarget() const noexcept
	{
		if (target_ == nullptr || !target_->IsActive())
		{
			return nullptr;
		}
		return target_;
	}

	// 记下的目标指针。已经停用时也返回，甩出那一帧还要它的位置。
	[[nodiscard]] ObjectBase* GetStoredTarget() const noexcept
	{
		return target_;
	}

	// 回收时丢掉目标指针。
	void OnDisable() override
	{
		target_ = nullptr;
	}

private:
	// 要跟着的目标。不拥有。
	ObjectBase* target_{ nullptr };
};

// 有约束就改目标，没有就装上一个。owner 是带着约束的物体，target 是目标。
inline void SetConstraintTarget(ObjectBase* owner, ObjectBase* target) noexcept
{
	if (owner == nullptr)
	{
		return;
	}
	if (ConstraintComponent* constraint = owner->GetComponent<ConstraintComponent>())
	{
		constraint->SetTarget(target);
		return;
	}
	owner->AddComponent<ConstraintComponent>(target);
}
