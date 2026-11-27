#pragma once
#include "IModule.h"
#include "AttackBase.h"

class Module_Attribute_SpeedRate : public IModule
{
public:
	Module_Attribute_SpeedRate(AttackBase* owner, float speedRate = 1.0f) noexcept
		:
		IModule(owner),
		speedRate_(speedRate)
	{}

	// 把配置倍率加进 speed.rate。直线弹出生和公转每帧都读 speed.Final()。
	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr || speedRate_ <= 0.0f)
		{
			return;
		}
		owner->Stats().speed.rate += speedRate_;
	}

	void OnRecycle() override {}

private:
	// 加到 speed.rate 上的数值。
	float speedRate_{ 1.0f };
};
