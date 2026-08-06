#pragma once

#include "AttackNodeLabel.h"

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

	[[nodiscard]] virtual bool HasAttackNodeLabel() const noexcept { return false; }
	[[nodiscard]] virtual AttackNodeLabel GetAttackNodeLabel() const noexcept
	{
		return AttackNodeLabel::Count;
	}

	[[nodiscard]] Attack* GetOwner() const noexcept { return owner_; }

private:
	Attack* owner_{ nullptr };
};
