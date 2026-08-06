#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

/**
 * @brief Update module: RequestDisable when owner life timer exceeds SetLifeTime.
 * @note Uses Attack::TickLifeTimer / ResetLifeTimer (same fields as legacy Attack::lifeTime).
 */
class Attribute_Lifetime_Module : public IProjectileModule
{
public:
	/**
	 * @param owner Injected by Attack::AddModule.
	 * @param durationSeconds If &gt; 0, writes owner SetLifeTime; otherwise keeps owner's current lifeTime.
	 */
	Attribute_Lifetime_Module(Attack* owner, float durationSeconds = -1.0f) noexcept
		:
		IProjectileModule(owner)
	{
		if (durationSeconds > 0.0f && owner != nullptr)
		{
			owner->SetLifeTime(durationSeconds);
		}
	}

	[[nodiscard]] bool HasAttackNodeLabel() const noexcept override { return true; }
	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Attribute_Lifetime;
	}

	void OnSpawn() override
	{
		if (Attack* owner = GetOwner())
		{
			owner->ResetLifeTimer();
		}
	}

	void OnUpdate(float dt) override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || !owner->IsActive())
		{
			return;
		}
		if (owner->TickLifeTimer(dt))
		{
			owner->RequestDisable();
		}
	}

	void OnRecycle() override
	{
		if (Attack* owner = GetOwner())
		{
			owner->ResetLifeTimer();
		}
	}
};
