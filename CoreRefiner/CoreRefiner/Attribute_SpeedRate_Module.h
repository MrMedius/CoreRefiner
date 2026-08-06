#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

/**
 * @brief Scales owner MoveAccel once when armed (after SpawnAt writes aim velocity).
 */
class Attribute_SpeedRate_Module : public IProjectileModule
{
public:
	Attribute_SpeedRate_Module(Attack* owner, float speedRate = 1.0f) noexcept
		:
		IProjectileModule(owner),
		speedRate_(speedRate)
	{}

	[[nodiscard]] bool HasAttackNodeLabel() const noexcept override { return true; }
	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Attribute_SpeedRate;
	}

	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || speedRate_ <= 0.0f)
		{
			return;
		}
		const XMFLOAT3 acc = owner->GetMoveAccel();
		owner->SetMoveAccel({
			acc.x * speedRate_,
			acc.y * speedRate_,
			acc.z * speedRate_
		});
	}

	void OnRecycle() override {}

private:
	float speedRate_{ 1.0f };
};
