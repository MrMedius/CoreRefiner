#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

class Attribute_SpeedRate_Module : public IProjectileModule
{
public:
	Attribute_SpeedRate_Module(Attack* owner, float speedRate = 1.0f) noexcept
		:
		IProjectileModule(owner),
		speedRate_(speedRate)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SpeedRate;
	}

	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || speedRate_ <= 0.0f)
		{
			return;
		}
		owner->Stats().speed.mul = speedRate_;
	}

	void OnRecycle() override {}

private:
	float speedRate_{ 1.0f };
};
