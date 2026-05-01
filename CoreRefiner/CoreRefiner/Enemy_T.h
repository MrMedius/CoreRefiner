#pragma once
#include "Enemy.h"

class Enemy_Red_0_IdleState;
class Enemy_Red_0_ChaseState;
class Enemy_Red_0_AttackState;
class Enemy_Red_0_HurtState;
class Enemy_Red_0_DeathState;

class Enemy_Red_0 : public Enemy
{
public:
	Enemy_Red_0(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, Object_Type_Tag tag = character_Enemy_T)
		:
		Enemy(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 3.0f, 3.0f, 0.0f });
		SetCollisionSize({ 1.5f, 2.5f, 2.5f });
		SetCollisionOnOff(true);
		SetHpMax(4.0f);
		ResetHpCurrent();
		SetMoveAccel(0.01f);
		SetAttackCollisionSize({ 10.0f,2.0f,5.0f });
		SetAttackInterval(2.5f);
		SetEnemyType(ENEMY_TYPE_RED);
		SetSearchArea({ 30.0f,10.0f,30.0f });

		// graphics init
		visualPre = std::make_unique<Sprite3D>(gfx, std::vector<std::string>{"asset\\Images\\\Enemy\\Enemy_Red_0.png"});
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->LinkTechniques(rg);

		// FSM state init
		FSM = std::make_unique<StateMachine<Enemy_Red_0>>(this);
		FSM->AddState(ENEMY_STATE[ENEMY_IDLE],	 std::make_unique<Enemy_Red_0_IdleState>());
		FSM->AddState(ENEMY_STATE[ENEMY_CHASE],  std::make_unique<Enemy_Red_0_ChaseState>());
		FSM->AddState(ENEMY_STATE[ENEMY_ATTACK], std::make_unique<Enemy_Red_0_AttackState>());
		FSM->AddState(ENEMY_STATE[ENEMY_HURT],	 std::make_unique<Enemy_Red_0_HurtState>());
		FSM->AddState(ENEMY_STATE[ENEMY_DEATH],  std::make_unique<Enemy_Red_0_DeathState>());

		// FSM state transitions init
		SetupTransitions();
		// init the state
		FSM->ChangeState(ENEMY_STATE[ENEMY_IDLE]);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, "wireBox");
		boxColliderWire->LinkTechniques(rg);
		searchColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 0.0f, 1.0f, 0.0f }, "wireSearch");
		searchColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override
	{
		Enemy::OnEnable();
		FSM->ChangeState(ENEMY_STATE[ENEMY_IDLE]);
	};
	void Update(float dt) override;
	void Submit(void) override;
	int GetKillScore() const noexcept override { return 40; }
private:
	void SetupTransitions(void) override;
private:
	std::unique_ptr<StateMachine<Enemy_Red_0>> FSM;
};

/*------------------------------------------------------------------------------
   Enemy_Red_0_IdleState
------------------------------------------------------------------------------*/
class Enemy_Red_0_IdleState : public State<Enemy_Red_0>
{
public:
	void OnEnter(Enemy_Red_0* owner) override;
	void OnExit(Enemy_Red_0* owner) override {};
	void Update(Enemy_Red_0* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_IDLE]; }
private:
	SpriteAnimeInfo pack{ 12, 8, 73, 24 };
};

/*------------------------------------------------------------------------------
   Enemy_Red_0_ChaseState
------------------------------------------------------------------------------*/
class Enemy_Red_0_ChaseState : public State<Enemy_Red_0>
{
public:
	void OnEnter(Enemy_Red_0* owner) override;
	void OnExit(Enemy_Red_0* owner) override {};
	void Update(Enemy_Red_0* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_CHASE]; }
private:
	SpriteAnimeInfo pack{ 12, 8, 73, 24 };
};

/*------------------------------------------------------------------------------
   Enemy_Red_0_AttackState
------------------------------------------------------------------------------*/
class Enemy_Red_0_AttackState : public State<Enemy_Red_0>
{
public:
	void OnEnter(Enemy_Red_0* owner) override;
	void OnExit(Enemy_Red_0* owner) override;
	void Update(Enemy_Red_0* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_ATTACK]; }
private:
	SpriteAnimeInfo pack{ 12, 8, 1, 24 };
	unsigned int FrameNoOld{ 0 };
	bool HaveHit{ false };
	XMFLOAT3 AttackVec{ 0.0f, 0.0f, 0.0f };
};

/*------------------------------------------------------------------------------
   Enemy_Red_0_HurtState
------------------------------------------------------------------------------*/
class Enemy_Red_0_HurtState : public State<Enemy_Red_0>
{
public:
	void OnEnter(Enemy_Red_0* owner) override;
	void OnExit(Enemy_Red_0* owner) override;
	void Update(Enemy_Red_0* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_HURT]; }
private:
	SpriteAnimeInfo pack{ 12, 8, 25, 24 };
};

/*------------------------------------------------------------------------------
   Enemy_Red_0_DeathState
------------------------------------------------------------------------------*/
class Enemy_Red_0_DeathState : public State<Enemy_Red_0>
{
public:
	void OnEnter(Enemy_Red_0* owner) override;
	void OnExit(Enemy_Red_0* owner) override {};
	void Update(Enemy_Red_0* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_DEATH]; }
private:
	SpriteAnimeInfo pack{ 12, 8, 49, 24 };
};