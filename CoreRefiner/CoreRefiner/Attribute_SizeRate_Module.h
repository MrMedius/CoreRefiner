#pragma once
#include "IProjectileModule.h"
#include "Attack.h"

class Attribute_SizeRate_Module : public IProjectileModule
{
public:
	Attribute_SizeRate_Module(Attack* owner, float sizeRate = 0.0f) noexcept
		:
		IProjectileModule(owner),
		sizeRate_(sizeRate)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SizeRate;
	}

	
	// 加算 size.rate；Ball::SpawnAt 在 ArmModules 之后用 size.Final() 调 ApplyPresentation。
	void OnSpawn() override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		owner->Stats().size.rate += sizeRate_;
	}

	void OnRecycle() override {}

private:
	float sizeRate_{ 0.0f };
};
