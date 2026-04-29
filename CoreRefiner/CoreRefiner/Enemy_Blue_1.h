#pragma once
#include "Enemy.h"

class Enemy_Blue_1_IdleState;
class Enemy_Blue_1_ChaseState;
class Enemy_Blue_1_AttackState;
class Enemy_Blue_1_HurtState;
class Enemy_Blue_1_DeathState;

class Enemy_Blue_1 : public Enemy
{
public:
	Enemy_Blue_1(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, Object_Type_Tag tag = character_Enemy_Blue_0)
		:
		Enemy(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 8.0f, 8.0f, 0.0f });
		SetCollisionSize({ 3.0f, 8.0f, 2.5f });
		SetCollisionOnOff(true);
		SetHpMax(7.0f);
		ResetHpCurrent();
		SetMoveAccel(0.03f);
		SetAttackCollisionSize({ 2.0f,8.0f,3.0f });
		SetAttackInterval(1.5f);
		SetEnemyType(ENEMY_TYPE_BLUE);
		SetSearchArea({ 100.0f,10.0f,100.0f });

		// graphics init
		visualPre = std::make_unique<Sprite3D>(gfx, std::vector<std::string>{"asset\\Images\\\Enemy\\Enemy_Blue_1.png"});
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->LinkTechniques(rg);

		// FSM state init
		FSM = std::make_unique<StateMachine<Enemy_Blue_1>>(this);
		FSM->AddState(ENEMY_STATE[ENEMY_IDLE],	 std::make_unique<Enemy_Blue_1_IdleState>());
		FSM->AddState(ENEMY_STATE[ENEMY_CHASE],  std::make_unique<Enemy_Blue_1_ChaseState>());
		FSM->AddState(ENEMY_STATE[ENEMY_ATTACK], std::make_unique<Enemy_Blue_1_AttackState>());
		FSM->AddState(ENEMY_STATE[ENEMY_HURT],	 std::make_unique<Enemy_Blue_1_HurtState>());
		FSM->AddState(ENEMY_STATE[ENEMY_DEATH],  std::make_unique<Enemy_Blue_1_DeathState>());

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
		attackColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, "wireAttack");
		attackColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override
	{
		Enemy::OnEnable();
		FSM->ChangeState(ENEMY_STATE[ENEMY_IDLE]);
	};
	void Update(float dt) override;
	void Submit(void) override;
	int GetKillScore() const noexcept override { return 70; }
private:
	void SetupTransitions(void) override;
private:
	std::unique_ptr<StateMachine<Enemy_Blue_1>> FSM;
};

/*------------------------------------------------------------------------------
   Enemy_Blue_1_IdleState
------------------------------------------------------------------------------*/
class Enemy_Blue_1_IdleState : public State<Enemy_Blue_1>
{
public:
	void OnEnter(Enemy_Blue_1* owner) override;
	void OnExit(Enemy_Blue_1* owner) override {};
	void Update(Enemy_Blue_1* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_IDLE]; }
private:
	SpriteAnimeInfo pack{ 12, 10, 73, 24 };
};

/*------------------------------------------------------------------------------
   Enemy_Blue_1_ChaseState
------------------------------------------------------------------------------*/
class Enemy_Blue_1_ChaseState : public State<Enemy_Blue_1>
{
public:
	void OnEnter(Enemy_Blue_1* owner) override;
	void OnExit(Enemy_Blue_1* owner) override {};
	void Update(Enemy_Blue_1* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_CHASE]; }
private:
	SpriteAnimeInfo pack{ 12, 10, 97, 24 };
};

/*------------------------------------------------------------------------------
   Enemy_Blue_1_AttackState
------------------------------------------------------------------------------*/
class Enemy_Blue_1_AttackState : public State<Enemy_Blue_1>
{
public:
	void OnEnter(Enemy_Blue_1* owner) override;
	void OnExit(Enemy_Blue_1* owner) override;
	void Update(Enemy_Blue_1* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_ATTACK]; }
private:
	SpriteAnimeInfo pack{ 12, 10, 1, 24 };
	unsigned int FrameNoOld{ 0 };
	bool HaveHit{ false };
};

/*------------------------------------------------------------------------------
   Enemy_Blue_1_HurtState
------------------------------------------------------------------------------*/
class Enemy_Blue_1_HurtState : public State<Enemy_Blue_1>
{
public:
	void OnEnter(Enemy_Blue_1* owner) override;
	void OnExit(Enemy_Blue_1* owner) override;
	void Update(Enemy_Blue_1* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_HURT]; }
private:
	SpriteAnimeInfo pack{ 12, 10, 25, 24 };
	Attack_Type_Tag BeAttackedTypeOld{ Attack_Type_None };
};

/*------------------------------------------------------------------------------
   Enemy_Blue_1_DeathState
------------------------------------------------------------------------------*/
class Enemy_Blue_1_DeathState : public State<Enemy_Blue_1>
{
public:
	void OnEnter(Enemy_Blue_1* owner) override;
	void OnExit(Enemy_Blue_1* owner) override {};
	void Update(Enemy_Blue_1* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_DEATH]; }
private:
	SpriteAnimeInfo pack{ 12, 10, 49, 24 };
};