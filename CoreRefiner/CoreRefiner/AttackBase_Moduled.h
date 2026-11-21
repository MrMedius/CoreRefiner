#pragma once
#include "AttackBase.h"
#include "IModule.h"
#include "AttackStepRecord.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// 带玩法模块的攻击。子弹走这一层。模块列表空着也可以，回收和命中仍按这一层走。
class AttackBase_Moduled : public AttackBase
{
public:
	explicit AttackBase_Moduled(Object_Type_Tag tag)
		:
		AttackBase(tag)
	{}

	// 先结算自己的命中，再按写入顺序跑模块的 OnHit。
	void OnCollide(CharacterBase* other) override final;

	// 先通知模块，再做对象回收，最后清掉模块。
	void Deactivate() override;
	// 先让还活着的子弹脱离并继承速度，再排队关掉自己。子弹不会跟着一起关掉。
	void RequestDisable() override;

	// 挂上一个玩法模块。T 必须继承 IModule，构造的第一个参数是这发弹。
	// 返回的指针不拥有模块，清模块时会失效。
	template <typename T, typename... Args>
	T* AddModule(Args&&... args)
	{
		static_assert(std::is_base_of_v<IModule, T>, "T must inherit from IModule");
		auto module = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* raw = module.get();
		modules_.push_back(std::move(module));
		return raw;
	}

	// 通知每个模块回收，然后丢掉它们。数值修正、配方和连发登记一起清掉。
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
		Stats().ResetMods();
		awaitingManagerAdopt_ = false;
		launchPosLocked_ = false;
		ClearLaunched_();
		assembledRecipe_.clear();
		adoptLive_ = {};
	}

	// 把当前装配步骤封到这发弹上。清模块时会丢掉。
	void SealAssembledRecipe(std::vector<AttackStepRecord> recipe)
	{
		assembledRecipe_ = std::move(recipe);
	}
	// 封存的装配步骤。复活时按这份列表重放。
	[[nodiscard]] const std::vector<AttackStepRecord>& GetAssembledRecipe() const noexcept
	{
		return assembledRecipe_;
	}

	// 锁住发射点。复活放弹时置位，返回模块就不再改位置。
	void SetLaunchPosLocked(bool locked) noexcept { launchPosLocked_ = locked; }
	[[nodiscard]] bool IsLaunchPosLocked() const noexcept { return launchPosLocked_; }

	// 登记谁来收养脱离父弹的子弹。连发放出时由攻击管理器挂上。
	void SetAdoptLive(std::function<void(AttackBase*)> fn)
	{
		adoptLive_ = std::move(fn);
	}
	// 把 live 交给已登记的收养回调。没登记就什么都不做。
	void AdoptLive(AttackBase* live)
	{
		if (adoptLive_)
		{
			adoptLive_(live);
		}
	}

	// 父弹卸下后，等攻击管理器把它收进更新列表。
	void SetAwaitingManagerAdopt(bool awaiting) noexcept { awaitingManagerAdopt_ = awaiting; }
	[[nodiscard]] bool IsAwaitingManagerAdopt() const noexcept { return awaitingManagerAdopt_; }

	// 对象池可拿走：已经不在场，并且模块已经清空。
	[[nodiscard]] bool IsReusable() const noexcept override
	{
		return !IsActive() && modules_.empty();
	}

	// 当前挂着的模块个数。
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

	// 有模块要求独立时先卸下父子，再跑各模块的 OnSpawn。
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
	// 跑每个模块的 OnSpawn。
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
	// 跑每个模块的 OnUpdate。
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
	// 按写入顺序跑每个模块的 OnHit。
	void DispatchOnHit(CharacterBase* other)
	{
		for (auto& module : modules_)
		{
			if (module != nullptr)
			{
				module->OnHit(other);
			}
		}
	}
	// 清模块之前通知每个模块即将关闭。
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

private:
	// 这发弹上的玩法模块。和对象组件列表分开。
	std::vector<std::unique_ptr<IModule>> modules_;
	// 是否正在等攻击管理器收养。
	bool awaitingManagerAdopt_{ false };
	// 发射点已锁住。返回模块不再改位置。
	bool launchPosLocked_{ false };
	// 封存的装配步骤。
	std::vector<AttackStepRecord> assembledRecipe_;
	// 收养脱离父弹的子弹。参数是那颗要活下去的弹。
	std::function<void(AttackBase*)> adoptLive_{};

	// 是否有模块要求开火时脱离父物体。
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

	// 记下世界坐标，继承父弹速度，脱离父物体，并交给收养回调。
	void DetachSelfFromParent_();
	// 让每个还活着的子弹子物体脱离。不关掉它们。
	void ReleaseLivingChildren_();
};
