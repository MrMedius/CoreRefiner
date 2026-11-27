#pragma once
#include "IModule.h"
#include "AttackBase.h"

class Module_Attribute_SizeRate : public IModule
{
public:
	Module_Attribute_SizeRate(AttackBase* owner, float sizeRate = 0.0f) noexcept
		:
		IModule(owner),
		sizeRate_(sizeRate)
	{}

	// 加算 size.rate。Ball::SpawnAt 在模块出生之后用 size.Final() 改缩放。
	void OnSpawn() override
	{
		AttackBase* owner = GetOwner();
		if (owner == nullptr)
		{
			return;
		}
		owner->Stats().size.rate += sizeRate_;
	}

	void OnRecycle() override {}

private:
	// 加到 size.rate 上的数值。
	float sizeRate_{ 0.0f };
};
