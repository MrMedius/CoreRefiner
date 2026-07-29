#pragma once
#include "Enemy.h"
#include "Enemy_T_Shape.h"
#include "VisualComponent.h"
#include "ColliderComponent.h"
#include "Channels.h"

class Enemy_T_ChaseState;
class Enemy_T_AttackState;
class Enemy_T_HurtState;
class Enemy_T_DeathState;

class Enemy_T : public Enemy
{
public:
	Enemy_T(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, Object_Type_Tag tag = character_Enemy_T)
		:
		Enemy(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 2.0f, 2.0f, 2.0f });
		SetHpMax(4.0f);
		ResetHpCurrent();
		SetMoveAccel(0.01f);
		SetAttackInterval(2.5f);

		// graphics init — VisualComponent owns the Drawable
		{
			auto shape = std::make_unique<Enemy_T_Shape>(gfx, GetSize());
			shape->LinkTechniques(rg);
			pVisual_ = AddComponent<VisualComponent>(
				std::move(shape), Chan::main | Chan::shadow, true);
		}

		// collision: register Box then write size
		pCollider_ = AddComponent<ColliderComponent>(
			Collider3D::CollideType::Box,
			ColliderSyncMode::FollowCenterAxisYFromRotation);
		pCollider_->SetCollisionSize(GetSize());
		pCollider_->SetEnabled(true);
		// Red Box wire — contrasts Player cyan Capsule / Ball green Sphere
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, "wireEnemyBox");

		// FSM state init
		FSM = std::make_unique<StateMachine<Enemy_T>>(this);
		FSM->AddState(ENEMY_STATE[ENEMY_CHASE],  std::make_unique<Enemy_T_ChaseState>());
		FSM->AddState(ENEMY_STATE[ENEMY_ATTACK], std::make_unique<Enemy_T_AttackState>());
		FSM->AddState(ENEMY_STATE[ENEMY_HURT],	 std::make_unique<Enemy_T_HurtState>());
		FSM->AddState(ENEMY_STATE[ENEMY_DEATH],  std::make_unique<Enemy_T_DeathState>());

		// FSM state transitions init
		SetupTransitions();
		// init the state
		FSM->ChangeState(ENEMY_STATE[ENEMY_CHASE]);
	}
	void OnEnable(void) override
	{
		Enemy::OnEnable();
		FSM->ChangeState(ENEMY_STATE[ENEMY_CHASE]);
	};
	void Update(float dt) override;
	void Submit(void) override;
	int GetKillScore() const noexcept override { return 40; }
	/**
	 * @brief Typed access to the owned Enemy_T_Shape for hurt/death animation.
	 */
	Enemy_T_Shape* GetVisual() noexcept
	{
		return pVisual_ != nullptr ? pVisual_->GetDrawableAs<Enemy_T_Shape>() : nullptr;
	}
private:
	void SetupTransitions(void) override;
private:
	std::unique_ptr<StateMachine<Enemy_T>> FSM;
	VisualComponent* pVisual_{ nullptr };
	ColliderComponent* pCollider_{ nullptr };
};


/*------------------------------------------------------------------------------
   Enemy_T_ChaseState
------------------------------------------------------------------------------*/
class Enemy_T_ChaseState : public State<Enemy_T>
{
public:
	void OnEnter(Enemy_T* owner) override;
	void OnExit(Enemy_T* owner) override {};
	void Update(Enemy_T* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_CHASE]; }
private:
};

/*------------------------------------------------------------------------------
   Enemy_T_AttackState
------------------------------------------------------------------------------*/
class Enemy_T_AttackState : public State<Enemy_T>
{
public:
	void OnEnter(Enemy_T* owner) override;
	void OnExit(Enemy_T* owner) override;
	void Update(Enemy_T* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_ATTACK]; }
private:
};

/*------------------------------------------------------------------------------
   Enemy_T_HurtState
------------------------------------------------------------------------------*/
class Enemy_T_HurtState : public State<Enemy_T>
{
public:
	void OnEnter(Enemy_T* owner) override;
	void OnExit(Enemy_T* owner) override;
	void Update(Enemy_T* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_HURT]; }
private:
};

/*------------------------------------------------------------------------------
   Enemy_T_DeathState
------------------------------------------------------------------------------*/
class Enemy_T_DeathState : public State<Enemy_T>
{
public:
	void OnEnter(Enemy_T* owner) override;
	void OnExit(Enemy_T* owner) override {};
	void Update(Enemy_T* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_DEATH]; }
private:
};