#pragma once

class Attack;
class Character;

/**
 * @brief Projectile gameplay module (separate from ObjectBase IComponent).
 * @note Wide hooks with empty defaults. Host calls all three channels; unused hooks no-op.
 *       Not for Visual/Collider — those stay on IComponent.
 */
class IProjectileModule
{
public:
	IProjectileModule() = delete;
	/**
	 * @brief Bind to the owning Attack (injected by Attack::AddModule).
	 */
	explicit IProjectileModule(Attack* owner) noexcept : owner_(owner) {}
	virtual ~IProjectileModule() = default;

	/** @brief Once when the shot is armed (Attack::SpawnAt dispatch). */
	virtual void OnSpawn() {}
	/** @brief Per-frame while the shot is active. */
	virtual void OnUpdate(float dt) { (void)dt; }
	/** @brief When the host reports a character collision. */
	virtual void OnHit(Character* other) { (void)other; }
	/**
	 * @brief Clear transient state before the module is dropped (pool recycle / ClearModules).
	 */
	virtual void OnRecycle() {}

	[[nodiscard]] Attack* GetOwner() const noexcept { return owner_; }

private:
	Attack* owner_{ nullptr };
};
