#pragma once

#include "ModuleNodeLabel.h"

class Attack;
class Character;

class IProjectileModule
{
public:
	IProjectileModule() = delete;
	explicit IProjectileModule(Attack* owner) noexcept : owner_(owner) {}
	virtual ~IProjectileModule() = default;

	virtual void OnSpawn() {}
	virtual void OnUpdate(float dt) { (void)dt; }
	virtual void OnHit(Character* other) { (void)other; }
	virtual void OnRecycle() {}

	[[nodiscard]] virtual bool HasModuleNodeLabel() const noexcept { return false; }
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept
	{
		return ModuleNodeLabel::Count;
	}

	[[nodiscard]] Attack* GetOwner() const noexcept { return owner_; }

private:
	Attack* owner_{ nullptr };
};
