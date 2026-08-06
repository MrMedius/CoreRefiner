#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

#include <cmath>

/**
 * @brief Update(+Spawn) module: orbit owner in local XZ about parent origin.
 */
class Rule_Orbit_Module : public IProjectileModule
{
public:
	/**
	 * @param owner Injected by Attack::AddModule.
	 * @param radius Local XZ orbit radius.
	 * @param angularSpeed Radians per second about parent Y.
	 * @param phase0 Initial angle in radians.
	 */
	Rule_Orbit_Module(Attack* owner, float radius, float angularSpeed, float phase0) noexcept
		:
		IProjectileModule(owner),
		radius_(radius),
		angularSpeed_(angularSpeed),
		phase0_(phase0),
		angle_(phase0)
	{}

	[[nodiscard]] bool HasAttackNodeLabel() const noexcept override { return true; }
	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Rule_Orbit;
	}

	void OnSpawn() override
	{
		angle_ = phase0_;
		ApplyLocalPose_();
	}

	void OnUpdate(float dt) override
	{
		angle_ += angularSpeed_ * dt;
		ApplyLocalPose_();
	}

	void OnRecycle() override
	{
		angle_ = phase0_;
	}

	void SetRadius(float radius) noexcept
	{
		radius_ = radius;
		ApplyLocalPose_();
	}

	void SetPhase0(float phase0) noexcept
	{
		phase0_ = phase0;
		angle_ = phase0;
		ApplyLocalPose_();
	}

private:
	void ApplyLocalPose_()
	{
		Attack* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		owner->SetLocalPosition({
			radius_ * std::cos(angle_),
			0.0f,
			radius_ * std::sin(angle_)
		});
	}

	float radius_{ 2.0f };
	float angularSpeed_{ 3.5f };
	float phase0_{ 0.0f };
	float angle_{ 0.0f };
};
