#pragma once
#include "IModule.h"
#include "Attack.h"

class Module_Attribute_SpeedRate : public IModule
{
public:
	Module_Attribute_SpeedRate(Attack* owner, float speedRate = 1.0f) noexcept
		:
		IModule(owner),
		speedRate_(speedRate)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SpeedRate;
	}

	// 把配置倍率加进 speed.mul；直线弹 SpawnAt、公转 OnUpdate 都读 Final()。
	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || speedRate_ <= 0.0f)
		{
			return;
		}
		owner->Stats().speed.rate += speedRate_;
	}

	void OnRecycle() override {}

private:
	float speedRate_{ 1.0f };
};
