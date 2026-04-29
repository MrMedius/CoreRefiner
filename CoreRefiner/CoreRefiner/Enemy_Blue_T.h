#pragma once
#include "Enemy.h"

class Enemy_Blue_T_IdleState;
class Enemy_Blue_T_HurtState;
class Enemy_Blue_T_DeathState;

class Enemy_Blue_T : public Enemy
{
public:
	Enemy_Blue_T(Graphics& gfx, Rgph::RenderGraph& rg, Object_Type_Tag tag = character_Enemy_Blue_T)
		:
		Enemy(tag)
	{
		// parameters init
		SetPosition(basePos);
		SetSize({ 5.0f, 10.0f, 0.0f });
		SetCollisionSize({ 4.0f, 0.5f, 2.0f });
		SetCollisionOnOff(true);
		SetHpMax(50.0f);
		ResetHpCurrent();
		SetMoveAccel(0.5f);
		SetEnemyType(ENEMY_TYPE_BLUE);

		// graphics init
		visualPre = std::make_unique<Sprite3D>(gfx, std::vector<std::string>{"asset\\Images\\\Enemy\\Enemy_Blue_T.png"});
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->SetFrame(1, 1, 1, 1, 1, 0);
		visualPre->LinkTechniques(rg);

		// FSM state init
		FSM = std::make_unique<StateMachine<Enemy_Blue_T>>(this);
		FSM->AddState(ENEMY_STATE[ENEMY_IDLE], std::make_unique<Enemy_Blue_T_IdleState>());
		FSM->AddState(ENEMY_STATE[ENEMY_HURT], std::make_unique<Enemy_Blue_T_HurtState>());
		FSM->AddState(ENEMY_STATE[ENEMY_DEATH], std::make_unique<Enemy_Blue_T_DeathState>());

		// FSM state transitions init
		SetupTransitions();
		// init the state
		FSM->ChangeState(ENEMY_STATE[ENEMY_IDLE]);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, "wireBox");
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override
	{
		Enemy::OnEnable();
		FSM->ChangeState(ENEMY_STATE[ENEMY_IDLE]);
		SetPosition(basePos);
		waveLimit = 10.0f;
		waveX = 0.0f;
		waveZ = 0.0f;
	};
	void Update(float dt) override;
	void Submit(void) override;
	int GetKillScore() const noexcept override { return 0; }
	void SetWaveLimit(float limit) { waveLimit = limit; }
	float GetWaveLimit(void) { return waveLimit; }
	void AddWaveX(float wave) { waveX += wave; }
	float GetWaveX(void) { return waveX; }
private:
	void SetupTransitions(void) override;
private:
	std::unique_ptr<StateMachine<Enemy_Blue_T>> FSM;
	static constexpr XMFLOAT3 basePos{ -10.0f,50.0f,10.0f };
	float waveLimit{ 10.0f };
	float waveX{ 0.0f };
	float waveZ{ 0.0f };
};

/*------------------------------------------------------------------------------
   Enemy_Blue_T_IdleState
------------------------------------------------------------------------------*/
class Enemy_Blue_T_IdleState : public State<Enemy_Blue_T>
{
public:
	void OnEnter(Enemy_Blue_T* owner) override;
	void OnExit(Enemy_Blue_T* owner) override {};
	void Update(Enemy_Blue_T* owner, float dt) override {};
	std::string GetName() const override { return ENEMY_STATE[ENEMY_IDLE]; }
};

/*------------------------------------------------------------------------------
   Enemy_Blue_T_HurtState
------------------------------------------------------------------------------*/
class Enemy_Blue_T_HurtState : public State<Enemy_Blue_T>
{
public:
	void OnEnter(Enemy_Blue_T* owner) override;
	void OnExit(Enemy_Blue_T* owner) override;
	void Update(Enemy_Blue_T* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_HURT]; }
private:
	Attack_Type_Tag BeAttackedTypeOld{ Attack_Type_None };
};

/*------------------------------------------------------------------------------
   Enemy_Blue_T_DeathState
------------------------------------------------------------------------------*/
class Enemy_Blue_T_DeathState : public State<Enemy_Blue_T>
{
public:
	void OnEnter(Enemy_Blue_T* owner) override;
	void OnExit(Enemy_Blue_T* owner) override {};
	void Update(Enemy_Blue_T* owner, float dt) override;
	std::string GetName() const override { return ENEMY_STATE[ENEMY_DEATH]; }
};