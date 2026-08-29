#pragma once
#include "ObjectBase.h"
#include "Character.h"
#include "IProjectileModule.h"
#include "Stats.h"

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

	/**
	 * @brief Immediate disable: cascade hierarchy, then recycle projectile modules.
	 */
	void Deactivate() override;

	/**
	 * @brief Attach a projectile module owned by this Attack.
	 * @tparam T Must derive from IProjectileModule; first ctor arg is always this owner.
	 * @return Non-owning pointer to the created module.
	 */
	template <typename T, typename... Args>
	T* AddModule(Args&&... args)
	{
		static_assert(std::is_base_of_v<IProjectileModule, T>, "T must inherit from IProjectileModule");
		auto module = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* raw = module.get();
		modules_.push_back(std::move(module));
		return raw;
	}

	/**
	 * @brief Call OnRecycle on each module then destroy them (pool-safe rebind).
	 */
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
	}

	/** @brief 本弹数值袋（damage / size / speed）。 */
	[[nodiscard]] AttackStats& Stats() noexcept { return stats_; }
	[[nodiscard]] const AttackStats& Stats() const noexcept { return stats_; }

	[[nodiscard]] std::size_t GetModuleCount() const noexcept { return modules_.size(); }

	template <typename T>
	T* GetModule() noexcept
	{
		static_assert(std::is_base_of_v<IProjectileModule, T>, "T must inherit from IProjectileModule");
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
		static_assert(std::is_base_of_v<IProjectileModule, T>, "T must inherit from IProjectileModule");
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

	/**
	 * @brief Write local position for modules (≈ Unity localPosition).
	 */
	void SetLocalPosition(XMFLOAT3 position) { SetPosition(position); }
	/**
	 * @brief Write local scale for modules (host SetSize).
	 */
	void SetLocalScale(XMFLOAT3 scale) { SetSize(scale); }

	/**
	 * @brief Shot lifetime seconds (Attribute_Lifetime_Module / recipes).
	 */
	void SetLifeTime(float seconds) { lifeTime = seconds; }
	[[nodiscard]] float GetLifeTime() const noexcept { return lifeTime; }
	/** @brief Reset elapsed life clock (call from SpawnAt / Attribute_Lifetime_Module::OnSpawn). */
	void ResetLifeTimer() { lastTime = 0.0f; }
	/**
	 * @brief Advance life clock; returns true when expired.
	 */
	[[nodiscard]] bool TickLifeTimer(float dt)
	{
		lastTime += dt;
		return lastTime >= lifeTime;
	}

	/**
	 * @brief DispatchOnSpawn + component sync (no motion).
	 * @note ModuleDeployer binds modules first; root SpawnAt arms root and flat children.
	 */
	void ArmModules()
	{
		DispatchOnSpawn();
		ObjectBase::Update(0.0f);
	}

protected:
	/** @brief Run OnSpawn on all bound modules (call from SpawnAt after pose reset). */
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
	/** @brief Run OnUpdate on all bound modules. */
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
	/** @brief Run OnHit on all bound modules. */
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

	/**
	 * @brief Update active hierarchy children (ObjectBase::Update is virtual).
	 */
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
	/**
	 * @brief Submit active hierarchy children.
	 */
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
	float lifeTime{ 0.5f };

private:
	/** @brief Gameplay modules; independent from ObjectBase IComponent list. */
	std::vector<std::unique_ptr<IProjectileModule>> modules_;
	AttackStats stats_{};
};
