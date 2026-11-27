#pragma once
#include "IModule.h"
#include "AttackBase.h"

class Module_Attribute_DamageRate : public IModule
{
public:
	Module_Attribute_DamageRate(AttackBase* owner, float damageRate = 0.0f) noexcept
		:
		IModule(owner),
		damageRate_(damageRate)
	{}

	// 加算 damage.rate。球命中时用 damage.Final() 扣血。
	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		owner->Stats().damage.rate += damageRate_;
	}

	void OnRecycle() override {}

private:
	// 加到 damage.rate 上的数值。
	float damageRate_{ 0.0f };
};
