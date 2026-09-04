#pragma once
#include "IModule.h"
#include "Attack.h"

/**
 * @brief Update module: RequestDisable when owner life timer exceeds SetLifeTime.
 * @note Uses Attack::TickLifeTimer / ResetLifeTimer (same fields as legacy Attack::lifeTime).
 */
class Module_Attribute_Lifetime : public IModule
{
public:
	/**
	 * @param owner Injected by Attack::AddModule.
	 * @param durationSeconds If &gt; 0, writes owner SetLifeTime; otherwise keeps owner's current lifeTime.
	 */
	Module_Attribute_Lifetime(Attack* owner, float durationSeconds = -1.0f) noexcept
		:
		IModule(owner)
	{
		if (durationSeconds > 0.0f && owner != nullptr)
		{
			owner->SetLifeTime(durationSeconds);
		}
	}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_Lifetime;
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
