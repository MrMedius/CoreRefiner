#pragma once
#include "IModule.h"
#include "AttackBase.h"

// 加算 lifetime.rate；开火后由 Ball 按拍板秒数倒计时，扣完 RequestDisable。
class Module_Attribute_LifetimeRate : public IModule
{
public:
	Module_Attribute_LifetimeRate(AttackBase* owner, float lifetimeRate = 0.5f) noexcept
		:
		IModule(owner),
		lifetimeRate_(lifetimeRate)
	{}

	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr || lifetimeRate_ <= 0.0f)
		{
			return;
		}
		owner->Stats().lifetime.rate += lifetimeRate_;
	}

	void OnRecycle() override {}

private:
	// 加到 lifetime.rate 上的数值。
	float lifetimeRate_{ 0.5f };
};
