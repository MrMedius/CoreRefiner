#pragma once
#include "AttackBase.h"
#include "IModule.h"
#include "AttackStepRecord.h"

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
	// 还没一起出生的子弹跟着关掉。已经交出的不在名单里，不会跟着消失。
	void Deactivate() override;

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

	// 通知每个模块回收，然后丢掉它们。数值修正和配方一起清掉。
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
		launchPosLocked_ = false;
		ClearLaunched_();
		assembledRecipe_.clear();
		// 模块清掉时同伴名单也丢掉。还没出生的同伴由 Deactivate 先关掉。
		bornWith_.clear();
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

	// 记下同一次出生的子弹。不是变换父子，不拥有。
	void AddBornWith(AttackBase_Moduled* ball)
	{
		if (ball == nullptr || ball == this)
		{
			return;
		}
		for (AttackBase_Moduled* existing : bornWith_)
		{
			if (existing == ball)
			{
				return;
			}
		}
		bornWith_.push_back(ball);
	}
	// 还没交出的同伴。出生前用它一起启动，也用来加仓库加成。
	[[nodiscard]] const std::vector<AttackBase_Moduled*>& GetBornWith() const noexcept
	{
		return bornWith_;
	}
	// 拿走同伴名单并清掉。调用方让它们自己更新。之后主体消失不会再碰到它们。
	[[nodiscard]] std::vector<AttackBase_Moduled*> TakeBornWith() noexcept
	{
		std::vector<AttackBase_Moduled*> out = std::move(bornWith_);
		bornWith_.clear();
		return out;
	}

	// 跑各模块的 OnSpawn。
	void ArmModules()
	{
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
	// 发射点已锁住。返回模块不再改位置。
	bool launchPosLocked_{ false };
	// 封存的装配步骤。复活时按这份列表重放。
	std::vector<AttackStepRecord> assembledRecipe_;
	// 同一次出生、还没交出的子弹。不是变换父子，不拥有。
	std::vector<AttackBase_Moduled*> bornWith_;
};
