#pragma once

class AttackBase;
class CharacterBase;

// 挂在一发带模组弹上的玩法。按装上的顺序出生、更新、命中和回收。
class IModule
{
public:
	IModule() = delete;
	explicit IModule(AttackBase* owner) noexcept : owner_(owner) {}
	virtual ~IModule() = default;

	// 这发弹出生时调用。用来对齐位置、写入数值或颜色。
	virtual void OnSpawn() {}
	// 这发弹每帧移动之后调用。dt 是这一帧的秒数。
	virtual void OnUpdate(float dt) { (void)dt; }
	// 这发弹自己的命中结算完之后调用。other 是被打中的角色。
	virtual void OnHit(CharacterBase* other) { (void)other; }
	// 模块被清掉时调用。对象池再拿这发弹之前，把本模块的状态归零。
	virtual void OnRecycle() {}
	// 宿主即将 Disable / ClearModules；Revive 在此放弹。默认空。
	virtual void OnDisable() {}

	// 带着这个模块的那发弹。
	[[nodiscard]] AttackBase* GetOwner() const noexcept { return owner_; }

private:
	// 宿主。不拥有。
	AttackBase* owner_{ nullptr };
};
