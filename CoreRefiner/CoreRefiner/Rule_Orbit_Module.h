#pragma once
#include "IProjectileModule.h"
#include "Attack.h"
#include "ColliderComponentBase.h"
#include "Player.h"
#include "XMath.h"

#include <algorithm>
#include <cmath>

/**
 * @brief 绕圆心公转：圆心只是指针，模块不改父子。
 * @note 实际半径 = max(节点半径, 圆心半径 + 自身半径 + 余量)。圆心是玩家时从 0 扩径。
 */
class Rule_Orbit_Module : public IProjectileModule
{
public:
	static constexpr float kClearance{ 2.0f };
	static constexpr float kExpandSeconds{ 0.3f };

	/**
	 * @param center 公转圆心（玩家或主弹）；空则不公转。
	 */
	Rule_Orbit_Module(Attack* owner, float radius, float phase0, ObjectBase* center = nullptr) noexcept
		:
		IProjectileModule(owner),
		radius_(radius),
		phase0_(phase0),
		angle_(phase0),
		center_(center)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Orbit;
	}

	[[nodiscard]] bool WantsDetachFromParent() const noexcept override { return true; }

	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}

		angle_ = phase0_;
		expandFromZero_ = IsPlayerCenter_();
		currentRadius_ = expandFromZero_ ? 0.0f : ComputeTargetRadius_(*owner);
		ApplyWorldPose_();
		RefreshColliderForExpand_();
	}

	void OnUpdate(float dt) override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		if (!IsCenterLive_())
		{
			center_ = nullptr;
			return;
		}

		const float omega = 1.0f + owner->Stats().speed.Final();
		angle_ += omega * dt;

		const float target = ComputeTargetRadius_(*owner);
		if (expandFromZero_ && currentRadius_ < target)
		{
			const float speed = (kExpandSeconds > 0.0f) ? (target / kExpandSeconds) : target;
			currentRadius_ += speed * dt;
			if (currentRadius_ >= target)
			{
				currentRadius_ = target;
				expandFromZero_ = false;
			}
		}
		else
		{
			currentRadius_ = target;
			expandFromZero_ = false;
		}
		ApplyWorldPose_();
		RefreshColliderForExpand_();
	}

	void OnRecycle() override
	{
		angle_ = phase0_;
		currentRadius_ = 0.0f;
		expandFromZero_ = false;
		center_ = nullptr;
		SetOwnerColliderEnabled_(true);
	}

	void SetRadius(float radius) noexcept
	{
		radius_ = radius;
	}

	void SetPhase0(float phase0) noexcept
	{
		phase0_ = phase0;
		angle_ = phase0;
	}

private:
	[[nodiscard]] bool IsCenterLive_() const noexcept
	{
		return center_ != nullptr && center_->IsActive();
	}

	[[nodiscard]] bool IsPlayerCenter_() const noexcept
	{
		return center_ != nullptr && center_->GetTag() == character_Player;
	}

	[[nodiscard]] static float BodyRadius_(const ObjectBase& body) noexcept
	{
		if (body.GetTag() == character_Player)
		{
			return PlayerCapsuleTuning::kRadius;
		}
		if (const auto* atk = dynamic_cast<const Attack*>(&body))
		{
			return atk->Stats().size.Final();
		}
		return 0.0f;
	}

	[[nodiscard]] float ComputeTargetRadius_(const Attack& owner) const noexcept
	{
		const float parentR = IsCenterLive_() ? BodyRadius_(*center_) : 0.0f;
		const float childR = owner.Stats().size.Final();
		const float floorR = parentR + childR + kClearance;
		return (std::max)(radius_, floorR);
	}

	void ApplyWorldPose_()
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || !IsCenterLive_())
		{
			return;
		}
		const XMFLOAT3 c = center_->GetWorldPosition();
		owner->SetLocalPosition(Vec3{
			c.x + currentRadius_ * std::cos(angle_),
			c.y,
			c.z + currentRadius_ * std::sin(angle_)
		});
	}

	void RefreshColliderForExpand_()
	{
		SetOwnerColliderEnabled_(!expandFromZero_);
	}

	void SetOwnerColliderEnabled_(bool enabled)
	{
		Attack* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		if (auto* col = owner->GetComponent<ColliderComponentBase>())
		{
			col->SetEnabled(enabled);
		}
	}

	float radius_{ 2.0f };
	float phase0_{ 0.0f };
	float angle_{ 0.0f };
	float currentRadius_{ 0.0f };
	bool expandFromZero_{ false };
	ObjectBase* center_{ nullptr };
};
