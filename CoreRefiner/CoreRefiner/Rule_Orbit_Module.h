#pragma once
#include "IProjectileModule.h"
#include "Attack.h"
#include "AttackManager.h"
#include "ColliderComponentBase.h"
#include "Player.h"
#include "XMath.h"

#include <algorithm>
#include <cmath>

/**
 * @brief 绕圆心公转：圆心只是指针，模块不改父子。
 * @note 实际半径 = max(节点半径, 圆心半径 + 自身半径 + 余量)。
 *       绕玩家时从出生极径边转到目标半径；径向加速度与普通发射相同。
 *       圆心失效时沿当下切线甩出，并继续沿该切线加速。
 */
class Rule_Orbit_Module : public IProjectileModule
{
public:
	static constexpr float kClearance{ 0.0f };

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
		if (IsPlayerCenter_())
		{
			pendingRadialStart_ = true;
			interpolating_ = true;
			currentRadius_ = 0.0f;
			radialVel_ = 0.0f;
			SetOwnerColliderEnabled_(false);
			return;
		}

		pendingRadialStart_ = false;
		interpolating_ = false;
		currentRadius_ = ComputeTargetRadius_(*owner);
		ApplyWorldPose_();
		SetOwnerColliderEnabled_(true);
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
			if (center_ != nullptr)
			{
				ReleaseToBallistic_(*owner, dt);
				center_ = nullptr;
			}
			return;
		}

		if (pendingRadialStart_)
		{
			SampleRadialStart_(*owner);
			pendingRadialStart_ = false;
		}

		const float omega = 1.0f + owner->Stats().speed.Final();
		angle_ += omega * dt;

		const float target = ComputeTargetRadius_(*owner);
		if (interpolating_)
		{
			const float shotAccel = AttackManager::kAimSpeed * owner->Stats().speed.Final();
			radialVel_ += shotAccel;
			const float diff = target - currentRadius_;
			if (std::abs(diff) <= radialVel_ || radialVel_ <= 0.0f)
			{
				currentRadius_ = target;
				interpolating_ = false;
				radialVel_ = 0.0f;
			}
			else
			{
				currentRadius_ += (diff > 0.0f) ? radialVel_ : -radialVel_;
			}
		}
		else
		{
			currentRadius_ = target;
		}
		ApplyWorldPose_();
		SetOwnerColliderEnabled_(!(interpolating_ && currentRadius_ < target));
		owner->ResetMoveVelocity();
	}

	void OnRecycle() override
	{
		angle_ = phase0_;
		currentRadius_ = 0.0f;
		radialVel_ = 0.0f;
		interpolating_ = false;
		pendingRadialStart_ = false;
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

	[[nodiscard]] float GetRadius() const noexcept { return radius_; }

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

	/**
	 * @brief 用当前世界坐标当极径起点（脚边→扩出，远处→收回）。
	 */
	void SampleRadialStart_(const Attack& owner)
	{
		const XMFLOAT3 c = center_->GetWorldPosition();
		const XMFLOAT3 p = owner.GetWorldPosition();
		const float dx = p.x - c.x;
		const float dz = p.z - c.z;
		currentRadius_ = std::sqrt(dx * dx + dz * dz);
		if (currentRadius_ > 1.0e-4f)
		{
			angle_ = std::atan2(dz, dx);
		}
		else
		{
			angle_ = phase0_;
			currentRadius_ = 0.0f;
		}

		const float target = ComputeTargetRadius_(owner);
		interpolating_ = std::abs(currentRadius_ - target) > 1.0e-4f;
		radialVel_ = 0.0f;
	}

	/**
	 * @brief 圆心刚失效：沿当下公转切线带上本帧弧长速度，加速度也沿该切线。
	 * @note 本帧 Ball 已用旧加速度位移，先扳回最后一圈上的切向一步，避免先朝开火方向冲。
	 */
	void ReleaseToBallistic_(Attack& owner, float dt)
	{
		const float stepDt = (dt > 1.0e-4f) ? dt : (1.0f / 60.0f);
		const float omega = 1.0f + owner.Stats().speed.Final();
		const float r = (currentRadius_ > 1.0e-4f) ? currentRadius_ : 0.0f;
		const float radialX = r * std::cos(angle_);
		const float radialZ = r * std::sin(angle_);

		XMFLOAT3 tangent{ -radialZ, 0.0f, radialX };
		if (!NormalizeXZ(tangent))
		{
			tangent = { -std::sin(angle_), 0.0f, std::cos(angle_) };
			if (!NormalizeXZ(tangent))
			{
				owner.ResetMoveVelocity();
				owner.SetMoveAccel({ 0.0f, 0.0f, 0.0f });
				return;
			}
		}

		const float arc = omega * r * stepDt;
		const XMFLOAT3 step = (V(tangent) * arc).ToFloat3();
		if (center_ != nullptr)
		{
			const XMFLOAT3 c = center_->GetWorldPosition();
			owner.SetLocalPosition(Vec3{
				c.x + radialX + step.x,
				c.y,
				c.z + radialZ + step.z
			});
		}

		owner.ResetMoveVelocity();
		owner.CalculateMoveVelocity(step);
		owner.SetMoveAccel(
			(V(tangent) * AttackManager::kAimSpeed * owner.Stats().speed.Final()).ToFloat3());
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
	float radialVel_{ 0.0f };
	bool interpolating_{ false };
	bool pendingRadialStart_{ false };
	ObjectBase* center_{ nullptr };
};
