#pragma once
#include "IModule.h"
#include "Attack.h"

/**
 * @brief 加算 lifetime.rate；开火后由 Ball 按拍板秒数倒计时，扣完 RequestDisable。
 */
class Module_Attribute_LifetimeRate : public IModule
{
public:
	Module_Attribute_LifetimeRate(Attack* owner, float lifetimeRate = 0.5f) noexcept
		:
		IModule(owner),
		lifetimeRate_(lifetimeRate)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_LifetimeRate;
	}

	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || lifetimeRate_ <= 0.0f)
		{
			return;
		}
		owner->Stats().lifetime.rate += lifetimeRate_;
	}

	void OnRecycle() override {}

private:
	float lifetimeRate_{ 0.5f };
};
