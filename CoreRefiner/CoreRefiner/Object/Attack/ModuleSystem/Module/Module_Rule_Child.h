#pragma once
#include "IModule.h"
#include "AttackBase_Moduled.h"
#include "ConstraintComponent.h"
#include "Framework/Helpers/Mathematics/XMath.h"

#include <cmath>

// 子物体跟随。出生时按主体的同伴名单站开，之后读约束上的目标做平移。
// 目标还在就加上这一帧的位移，并记下它的速度和加速度。
// 目标刚没时用记下的速度放手，不读主体被命中清掉之后的值。
// 放手这一帧先留着约束上的旧目标，下一帧再清掉。后装的公转同一帧还能读到它。
class Module_Rule_Child : public IModule
{
public:
	explicit Module_Rule_Child(AttackBase* owner) noexcept
		:
		IModule(owner)
	{}

	// 先按同伴名单站开，再对齐锚点。主体的模块已经跑完，返回造成的位移能读到。
	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		ConstraintComponent* constraint = owner->GetComponent<ConstraintComponent>();
		if (constraint == nullptr)
		{
			return;
		}
		ObjectBase* live = constraint->GetTarget();
		if (live == nullptr)
		{
			return;
		}
		PlaceOnRing_(*owner, *live);
		anchor_ = live->GetWorldPosition();
		anchorValid_ = true;
		RememberMotion_(live);
	}

	// 目标还在就平移，并记下它此刻的速度。刚没时用记下的速度放手。
	void OnUpdate(float dt) override
	{
		(void)dt;
		if (released_)
		{
			// 上一帧已经放手。这时公转已经用过旧目标，可以清掉。
			if (clearConstraintNext_)
			{
				AttackBase* owner = GetOwner();
				if (owner != nullptr)
				{
					if (ConstraintComponent* constraint = owner->GetComponent<ConstraintComponent>())
					{
						constraint->SetTarget(nullptr);
					}
				}
				clearConstraintNext_ = false;
			}
			return;
		}
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		ConstraintComponent* constraint = owner->GetComponent<ConstraintComponent>();
		if (constraint == nullptr)
		{
			return;
		}

		if (ObjectBase* live = constraint->GetTarget())
		{
			if (anchorValid_)
			{
				ApplyDelta_(*owner, live->GetWorldPosition());
			}
			anchor_ = live->GetWorldPosition();
			anchorValid_ = true;
			RememberMotion_(live);
			return;
		}

		ObjectBase* stored = constraint->GetStoredTarget();
		if (stored == nullptr)
		{
			return;
		}
		if (anchorValid_)
		{
			ApplyDelta_(*owner, stored->GetWorldPosition());
		}
		// 这一帧自己的位移已经加过。用上一帧记下的速度，留给下一帧。
		// 不读主体现在的速度：被命中时它已经先被清成 0。
		if (motionValid_)
		{
			owner->ResetMoveVelocity();
			owner->CalculateMoveVelocity(carriedVelocity_);
			owner->SetMoveAccel(carriedAccel_);
		}
		released_ = true;
		// 这一帧先留着旧目标。下一帧再清，后装的公转还要用它甩出。
		clearConstraintNext_ = true;
	}

	// 清掉锚点、放手标记和记下的速度，对象池再拿时重新跟随。
	void OnRecycle() override
	{
		anchor_ = {};
		anchorValid_ = false;
		released_ = false;
		clearConstraintNext_ = false;
		motionValid_ = false;
		carriedVelocity_ = {};
		carriedAccel_ = {};
	}

private:
	// 没有被公转盖掉时，子物体离主体的水平距离。
	static constexpr float kRingSpacing{ 2.0f };

	// 按自己在主体同伴名单里的序号站开。core 是约束上还在场的目标。
	// 目标不是带模组的攻击，或名单里没有自己时，叠在目标上。
	void PlaceOnRing_(AttackBase& owner, ObjectBase& core)
	{
		const XMFLOAT3 at = core.GetWorldPosition();
		float offsetX = 0.0f;
		float offsetZ = 0.0f;
		if (auto* moduled = dynamic_cast<AttackBase_Moduled*>(&core))
		{
			const std::vector<AttackBase_Moduled*>& born = moduled->GetBornWith();
			const std::size_t n = born.size();
			for (std::size_t i = 0; i < n; ++i)
			{
				if (born[i] == &owner)
				{
					// phase 是这一颗在整圈上的角度。
					const float phase = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(n);
					offsetX = kRingSpacing * std::cos(phase);
					offsetZ = kRingSpacing * std::sin(phase);
					break;
				}
			}
		}
		owner.SetLocalPosition({
			at.x + offsetX,
			at.y,
			at.z + offsetZ
		});
	}

	// 从还在场上的攻击目标抄下速度和加速度。target 不是攻击时什么都不记。
	void RememberMotion_(ObjectBase* target) noexcept
	{
		auto* core = dynamic_cast<AttackBase*>(target);
		if (core == nullptr)
		{
			return;
		}
		carriedVelocity_ = core->GetMoveVelocity();
		carriedAccel_ = core->GetMoveAccel();
		motionValid_ = true;
	}

	// 把目标从锚点到 now 的位移加到宿主上。now 是目标当前的世界位置。
	void ApplyDelta_(AttackBase& owner, const XMFLOAT3& now)
	{
		const XMFLOAT3 pos = owner.GetPosition();
		owner.SetLocalPosition({
			pos.x + (now.x - anchor_.x),
			pos.y + (now.y - anchor_.y),
			pos.z + (now.z - anchor_.z)
		});
	}

	// 上一帧目标的世界位置。只用于平移，不含缩放。
	XMFLOAT3 anchor_{};
	// 锚点已在出生时对齐。没对齐的那一帧只记下位置，不加位移。
	bool anchorValid_{ false };
	// 已经放手。之后不再跟随，也不再接速度。
	bool released_{ false };
	// 放手的这一帧先留着旧目标。下一帧再清掉，给后装的公转用。
	bool clearConstraintNext_{ false };
	// 已经从目标抄下过速度。没抄下时放手不改自己的速度。
	bool motionValid_{ false };
	// 目标还在时记下的速度。放手时交给自己。
	XMFLOAT3 carriedVelocity_{};
	// 目标还在时记下的加速度。放手时交给自己。
	XMFLOAT3 carriedAccel_{};
};
