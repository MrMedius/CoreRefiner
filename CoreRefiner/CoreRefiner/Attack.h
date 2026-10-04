#pragma once
#include "ObjectBase.h"
#include "Character.h"
#include "IModule.h"
#include "AttackStepRecord.h"
#include "Stats.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class Attack : public ObjectBase
{
public:
	Attack(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	void virtual SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) = 0;
	void OnEnable(void) override = 0;
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	virtual void OnCollide(Character* other) = 0;

	// Immediate disable: cascade hierarchy, then recycle projectile modules.
	// Reset / 池回收仍级联子物体；玩法销毁请走 RequestDisable。
	void Deactivate() override;

	// 先把仍存活的子弹卸下并继承速度，再排队销毁自身（不级联子弹）。
	void RequestDisable() override;

	// Attach a projectile module owned by this Attack.
	// T Must derive from IModule; first ctor arg is always this owner.
	// Non-owning pointer to the created module.
	template <typename T, typename... Args>
	T* AddModule(Args&&... args)
	{
		static_assert(std::is_base_of_v<IModule, T>, "T must inherit from IModule");
		auto module = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* raw = module.get();
		modules_.push_back(std::move(module));
		return raw;
	}

	// Call OnRecycle on each module then destroy them (pool-safe rebind).
	void ClearModules()
	{
		for (auto& module : modules_)
		{
			if (module != nullptr)
			{
				module->OnRecycle();
			}
		}
		modules_.clear();
		stats_.ResetMods();
		awaitingManagerAdopt_ = false;
		launchPosLocked_ = false;
		assembledRecipe_.clear();
		adoptLive_ = {};
	}

	// FlushStandby 时把当前流水线 Step 快照封到根弹（池化 ClearModules 会清掉）。
	void SealAssembledRecipe(std::vector<AttackStepRecord> recipe)
	{
		assembledRecipe_ = std::move(recipe);
	}
	[[nodiscard]] const std::vector<AttackStepRecord>& GetAssembledRecipe() const noexcept
	{
		return assembledRecipe_;
	}

	// 本弹数值袋（damage / size / speed / lifetime）。
	[[nodiscard]] AttackStats& Stats() noexcept { return stats_; }
	[[nodiscard]] const AttackStats& Stats() const noexcept { return stats_; }

	// 锁发射点；Return 等后续 Node 读，Revive 放弹时置位。本步 FireRoots 仍 SpawnAt。
	void SetLaunchPosLocked(bool locked) noexcept { launchPosLocked_ = locked; }
	[[nodiscard]] bool IsLaunchPosLocked() const noexcept { return launchPosLocked_; }

	// 登记「把停放弹纳入活体列表」的回调（FireRoots / AttackManager::AdoptLive 挂上）。
	void SetAdoptLive(std::function<void(Attack*)> fn)
	{
		adoptLive_ = std::move(fn);
	}
	// 把 live 交给已登记回调；未登记则 no-op。
	void AdoptLive(Attack* live)
	{
		if (adoptLive_)
		{
			adoptLive_(live);
		}
	}

	// 父弹卸下后等待 AttackManager 收进更新列表。
	void SetAwaitingManagerAdopt(bool awaiting) noexcept { awaitingManagerAdopt_ = awaiting; }
	[[nodiscard]] bool IsAwaitingManagerAdopt() const noexcept { return awaitingManagerAdopt_; }

	// 对象池可拿走：已非 Active，且玩法模块已清空（OnDisable 尚未 ClearModules 时不可复用）。
	[[nodiscard]] bool IsReusable() const noexcept override
	{
		return !IsActive() && modules_.empty();
	}

	[[nodiscard]] std::size_t GetModuleCount() const noexcept { return modules_.size(); }

	template <typename T>
	T* GetModule() noexcept
	{
		static_assert(std::is_base_of_v<IModule, T>, "T must inherit from IModule");
		for (auto& module : modules_)
		{
			if (T* typed = dynamic_cast<T*>(module.get()))
			{
				return typed;
			}
		}
		return nullptr;
	}
	template <typename T>
	const T* GetModule() const noexcept
	{
		static_assert(std::is_base_of_v<IModule, T>, "T must inherit from IModule");
		for (const auto& module : modules_)
		{
			if (const T* typed = dynamic_cast<const T*>(module.get()))
			{
				return typed;
			}
		}
		return nullptr;
	}

public:
	void CalculateMoveVelocity(float X, float Y, float Z) { MoveVelocity = (V(MoveVelocity) + Vec3{ X, Y, Z }).ToFloat3(); }
	void CalculateMoveVelocity(XMFLOAT3 offset) { MoveVelocity = (V(MoveVelocity) + V(offset)).ToFloat3(); }
	void ResetMoveVelocity(void) { MoveVelocity = { 0.0f,0.0f,0.0f }; }
	XMFLOAT3 GetMoveVelocity(void) const { return MoveVelocity; }
	void SetMoveAccel(XMFLOAT3 accel) { MoveAccel = accel; }
	XMFLOAT3 GetMoveAccel(void) const { return MoveAccel; }

	// Write local position for modules (≈ Unity localPosition).
	void SetLocalPosition(XMFLOAT3 position) { SetPosition(position); }
	// Write local scale for modules (host SetSize).
	void SetLocalScale(XMFLOAT3 scale) { SetSize(scale); }

	// 开火时拍板的寿命秒数（ArmModules 之后 = lifetime.Final()）。
	void SetLifeTime(float seconds) { lifeTime = seconds; }
	[[nodiscard]] float GetLifeTime() const noexcept { return lifeTime; }
	// Reset elapsed life clock (call from SpawnAt).
	void ResetLifeTimer() { lastTime = 0.0f; }
	// Advance life clock; returns true when expired.
	[[nodiscard]] bool TickLifeTimer(float dt)
	{
		lastTime += dt;
		return lastTime >= lifeTime;
	}

	// 若有模块要求独立，先卸父子，再 DispatchOnSpawn + 组件同步。
	// 装配树仍由 Core_Ball / Spawn_Ball 建立；开火后需要独立世界坐标的弹在此摘下。
	void ArmModules()
	{
		if (WantsDetachFromParent_())
		{
			DetachSelfFromParent_();
		}
		DispatchOnSpawn();
		ObjectBase::Update(0.0f);
	}

protected:
	// Run OnSpawn on all bound modules (call from SpawnAt after pose reset).
	void DispatchOnSpawn()
	{
		for (auto& module : modules_)
		{
			if (module != nullptr)
			{
				module->OnSpawn();
			}
		}
	}
	// Run OnUpdate on all bound modules.
	void DispatchOnUpdate(float dt)
	{
		for (auto& module : modules_)
		{
			if (module != nullptr)
			{
				module->OnUpdate(dt);
			}
		}
	}
	// Run OnHit on all bound modules.
	void DispatchOnHit(Character* other)
	{
		for (auto& module : modules_)
		{
			if (module != nullptr)
			{
				module->OnHit(other);
			}
		}
	}
	// Run OnOwnerWillDisable on all bound modules（ClearModules 之前）。
	void DispatchOnDisable()
	{
		for (auto& module : modules_)
		{
			if (module != nullptr)
			{
				module->OnDisable();
			}
		}
	}

	// Update active hierarchy children (ObjectBase::Update is virtual).
	void UpdateChildren(float dt)
	{
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i); child != nullptr && child->IsActive())
			{
				child->Update(dt);
			}
		}
	}
	// Submit active hierarchy children.
	void SubmitChildren()
	{
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i); child != nullptr && child->IsActive())
			{
				child->Submit();
			}
		}
	}

protected:
	XMFLOAT3 MoveAccel{ 0.0f,0.0f,0.0f };
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };
	float lastTime{ 0.0f };
	float lifeTime{ 2.0f };

private:
	// Gameplay modules; independent from ObjectBase IComponent list.
	std::vector<std::unique_ptr<IModule>> modules_;
	AttackStats stats_{};
	bool awaitingManagerAdopt_{ false };
	bool launchPosLocked_{ false };
	std::vector<AttackStepRecord> assembledRecipe_{};
	std::function<void(Attack*)> adoptLive_{};

	// 任一模块要求开火时从父物体独立。
	[[nodiscard]] bool WantsDetachFromParent_() const noexcept
	{
		for (const auto& module : modules_)
		{
			if (module != nullptr && module->WantsDetachFromParent())
			{
				return true;
			}
		}
		return false;
	}

	// 烘焙世界坐标、继承父弹速度、ClearParent，并交给 AdoptLive / AM 收养标记。
	void DetachSelfFromParent_();

	// 卸下仍存活的子弹：烘焙世界坐标、继承本弹速度、ClearParent。
	void ReleaseLivingChildren_();
};
