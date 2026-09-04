#pragma once
#include "IModule.h"
#include "Attack.h"

class Module_Attribute_DamageRate : public IModule
{
public:
	Module_Attribute_DamageRate(Attack* owner, float damageRate = 0.0f) noexcept
		:
		IModule(owner),
		damageRate_(damageRate)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_DamageRate;
	}

	// 加算 damage.rate；Ball::OnCollide 用 damage.Final() 扣血。
	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		owner->Stats().damage.rate += damageRate_;
	}

	void OnRecycle() override {}

private:
	float damageRate_{ 0.0f };
};
