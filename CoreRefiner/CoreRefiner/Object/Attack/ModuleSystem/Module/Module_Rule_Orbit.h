#pragma once
#include "IModule.h"
#include "AttackBase.h"
#include "AttackBase_Moduled.h"
#include "AttackManager.h"
#include "ColliderComponentBase.h"
#include "ConstraintComponent.h"
#include "Player.h"
#include "Framework/Helpers/Mathematics/XMath.h"

#include <algorithm>
#include <cmath>

// 绕约束目标公转。圆心每帧从约束上读，模块自己不保存。
// 实际半径 = max(节点半径, 圆心半径 + 自身半径 + 余量)。
// 绕玩家时从出生极径边转到目标半径；径向加速度与普通发射相同。
// 目标没了就沿当下切线甩出，并继续沿该切线加速。
class Module_Rule_Orbit : public IModule
{
public:
	static constexpr float kClearance{ 0.0f };

	// radius 是节点半径。phase0 是节点上的起始角，负值表示自动，留到 OnSpawn 再算。圆心在宿主的约束上。
	Module_Rule_Orbit(AttackBase* owner, float radius, float phase0) noexcept
		:
		IModule(owner),
		radius_(radius),
		phase0_(phase0),
		angle_(phase0)
	{}

	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}

		// 起始角为负表示自动。按同伴名单算好再往下摆位置。
		if (phase0_ < 0.0f)
		{
			phase0_ = ComputeAutoPhase_();
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
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		if (!IsCenterLive_())
		{
			// 目标刚停用：沿切线甩出一次，然后清掉指针，下一帧不再甩。
			if (StoredCenter_() != nullptr)
			{
				ReleaseToBallistic_(*owner, dt);
				ClearCenter_();
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
		SetOwnerColliderEnabled_(true);
	}

private:
	// 起始角为负时，按约束目标的同伴名单算角度。绕玩家或名单里没有自己时用 0。
	[[nodiscard]] float ComputeAutoPhase_() const noexcept
	{
		AttackBase* owner = GetOwner();
		ObjectBase* center = LiveCenter_();
		auto* core = dynamic_cast<AttackBase_Moduled*>(center);
		if (owner == nullptr || core == nullptr)
		{
			return 0.0f;
		}
		const std::vector<AttackBase_Moduled*>& born = core->GetBornWith();
		const std::size_t n = born.size();
		if (n == 0u)
		{
			return 0.0f;
		}
		for (std::size_t i = 0; i < n; ++i)
		{
			if (born[i] == owner)
			{
				return DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(n);
			}
		}
		return 0.0f;
	}

	// 宿主上的约束。没有就返回空。
	[[nodiscard]] ConstraintComponent* Constraint_() const noexcept
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return nullptr;
		}
		return owner->GetComponent<ConstraintComponent>();
	}

	// 约束上还在场的目标。
	[[nodiscard]] ObjectBase* LiveCenter_() const noexcept
	{
		ConstraintComponent* constraint = Constraint_();
		if (constraint == nullptr)
		{
			return nullptr;
		}
		return constraint->GetTarget();
	}

	// 约束记下的指针。目标停用时也要，甩出那一帧取它的位置。
	[[nodiscard]] ObjectBase* StoredCenter_() const noexcept
	{
		ConstraintComponent* constraint = Constraint_();
		if (constraint == nullptr)
		{
			return nullptr;
		}
		return constraint->GetStoredTarget();
	}

	// 甩出之后清掉约束目标，避免下一帧再甩一次。
	void ClearCenter_() noexcept
	{
		if (ConstraintComponent* constraint = Constraint_())
		{
			constraint->SetTarget(nullptr);
		}
	}

	[[nodiscard]] bool IsCenterLive_() const noexcept
	{
		return LiveCenter_() != nullptr;
	}

	[[nodiscard]] bool IsPlayerCenter_() const noexcept
	{
		ObjectBase* center = LiveCenter_();
		return center != nullptr && center->GetTag() == character_Player;
	}

	[[nodiscard]] static float BodyRadius_(const ObjectBase& body) noexcept
	{
		if (body.GetTag() == character_Player)
		{
			return PlayerCapsuleTuning::kRadius;
		}
		if (const auto* atk = dynamic_cast<const AttackBase*>(&body))
		{
			return atk->Stats().size.Final();
		}
		return 0.0f;
	}

	[[nodiscard]] float ComputeTargetRadius_(const AttackBase& owner) const noexcept
	{
		const ObjectBase* center = LiveCenter_();
		const float parentR = (center != nullptr) ? BodyRadius_(*center) : 0.0f;
		const float childR = owner.Stats().size.Final();
		const float floorR = parentR + childR + kClearance;
		return (std::max)(radius_, floorR);
	}

	// 用当前世界坐标当极径起点（脚边→扩出，远处→收回）。
	void SampleRadialStart_(const AttackBase& owner)
	{
		ObjectBase* center = LiveCenter_();
		if (center == nullptr)
		{
			return;
		}
		const XMFLOAT3 c = center->GetWorldPosition();
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

	// 圆心刚失效：沿当下公转切线带上本帧弧长速度，加速度也沿该切线。
	// 本帧 Ball 已用旧加速度位移，先扳回最后一圈上的切向一步，避免先朝开火方向冲。
	void ReleaseToBallistic_(AttackBase& owner, float dt)
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
		if (ObjectBase* center = StoredCenter_())
		{
			const XMFLOAT3 c = center->GetWorldPosition();
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
		AttackBase* owner = GetOwner();
		if (owner == nullptr || !IsCenterLive_())
		{
			return;
		}
		ObjectBase* center = LiveCenter_();
		if (center == nullptr)
		{
			return;
		}
		const XMFLOAT3 c = center->GetWorldPosition();
		owner->SetLocalPosition(Vec3{
			c.x + currentRadius_ * std::cos(angle_),
			c.y,
			c.z + currentRadius_ * std::sin(angle_)
		});
	}

	void SetOwnerColliderEnabled_(bool enabled)
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		if (auto* col = owner->GetComponent<ColliderComponentBase>())
		{
			col->SetEnabled(enabled);
		}
	}

	// 节点传入的公转半径。实际飞行半径还要和双方体积取较大值。
	float radius_{ 2.0f };
	// 起始角。负值表示自动，出生时再算。
	float phase0_{ 0.0f };
	float angle_{ 0.0f };
	float currentRadius_{ 0.0f };
	float radialVel_{ 0.0f };
	bool interpolating_{ false };
	bool pendingRadialStart_{ false };
};
