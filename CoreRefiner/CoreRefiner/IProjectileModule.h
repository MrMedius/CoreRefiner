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
	/** @brief 宿主即将 Disable / ClearModules；Revive 在此放弹。默认空。 */
	virtual void OnOwnerWillDisable() {}

	/**
	 * @brief 开火 ArmModules 时是否从父物体卸下（独立世界坐标，由 Attack 执行）。
	 */
	[[nodiscard]] virtual bool WantsDetachFromParent() const noexcept { return false; }

	[[nodiscard]] virtual bool HasModuleNodeLabel() const noexcept { return false; }
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept
	{
		return ModuleNodeLabel::Count;
	}

	[[nodiscard]] Attack* GetOwner() const noexcept { return owner_; }

private:
	Attack* owner_{ nullptr };
};
