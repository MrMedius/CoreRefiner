#pragma once

#include "ModuleNodeLabel.h"

class AttackBase;
class CharacterBase;

class IModule
{
public:
	IModule() = delete;
	explicit IModule(AttackBase* owner) noexcept : owner_(owner) {}
	virtual ~IModule() = default;

	virtual void OnSpawn() {}
	virtual void OnUpdate(float dt) { (void)dt; }
	virtual void OnHit(CharacterBase* other) { (void)other; }
	virtual void OnRecycle() {}
	// 宿主即将 Disable / ClearModules；Revive 在此放弹。默认空。
	virtual void OnDisable() {}

	[[nodiscard]] virtual bool HasModuleNodeLabel() const noexcept { return false; }
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept
	{
		return ModuleNodeLabel::Count;
	}

	[[nodiscard]] AttackBase* GetOwner() const noexcept { return owner_; }

private:
	AttackBase* owner_{ nullptr };
};
