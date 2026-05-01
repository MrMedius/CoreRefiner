#pragma once
#include "Character.h"
#include "RenderGraph.h"
#include "ObjectCodex.h"
#include "CameraContainer.h"
#include "Player_Shape.h"

// プレイヤー状態のID
enum PLAYER_STATE_ID {
	PLAYER_IDLE,
	PLAYER_MOVE,
	PLAYER_ATTACK,
	PLAYER_HURT,
	PLAYER_DEATH,
};
// プレイヤー状態の名前
static const std::string PLAYER_STATE[] = {
	"PLAYER_IDLE",
	"PLAYER_MOVE",
	"PLAYER_ATTACK",
	"PLAYER_HURT",
	"PLAYER_DEATH",
};
class Player_IdleState;
class Player_MoveState;
class Player_AttackState;
class Player_HurtState;
class Player_DeathState;

class Player : public Character
{
public:
	Player(Graphics& gfx, Rgph::RenderGraph& rg, CameraContainer* camera, XMFLOAT3 position, Object_Type_Tag tag = character_Player)
		:
		Character(tag),
		Gfx(gfx),
		Rg(rg),
		pCamera(camera)
	{
		// control init
		BindPad(0);

		// parameters init
		SetPosition(position);
		SetSize({ 5.0f, 5.0f, 0.0f });
		SetCollisionSize({ 1.5f, 5.0f, 2.0f });
		SetCollisionOnOff(true);
		SetHpMax(30.0f);
		ResetHpCurrent();
		SetMoveAccel(0.035f);
		SetAttackCollisionSize({ 4.0f,6.0f,2.5f });
		SetAttackInterval(1.5f);

		// graphics init
		visualPre_Head = std::make_unique<Player_Head>(gfx, XMFLOAT3{ 3.0f, 3.0f, 3.0f });
		visualPre_Head->SetPosition(transInfo.position);
		visualPre_Head->LinkTechniques(rg);

		visualPre_Body = std::make_unique<Player_Body>(gfx, XMFLOAT3{ 3.0f, 3.0f, 3.0f });
		visualPre_Body->SetPosition(transInfo.position);
		visualPre_Body->LinkTechniques(rg);

		// FSM state init
		FSM = std::make_unique<StateMachine<Player>>(this);
		FSM->AddState(PLAYER_STATE[PLAYER_IDLE],	std::make_unique<Player_IdleState>());
		FSM->AddState(PLAYER_STATE[PLAYER_MOVE],	std::make_unique<Player_MoveState>());
		FSM->AddState(PLAYER_STATE[PLAYER_ATTACK],	std::make_unique<Player_AttackState>());
		FSM->AddState(PLAYER_STATE[PLAYER_HURT],	std::make_unique<Player_HurtState>());
		FSM->AddState(PLAYER_STATE[PLAYER_DEATH],	std::make_unique<Player_DeathState>());

		// FSM state transitions init
		SetupTransitions();
		// init the state
		FSM->ChangeState(PLAYER_STATE[PLAYER_IDLE]);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, "wireBox");
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override
	{
		FSM->ChangeState(PLAYER_STATE[PLAYER_IDLE]);
		SetPosition(XMFLOAT3(0.0f, 45.0f, 0.0f));
		MoveVelocity = { 0.0f,0.0f,0.0f };
		ResetHpCurrent();
		SetIsDeath(false);
		SetIsGameOver(false);
		IsGameOver = false;
		SetAttackCollisionSize({ 4.0f,6.0f,2.5f });
		SetAttackCollisionOnOff(false);
	}
	void Update(float dt) override;
	void Submit(void) override;
	float GetHpDrawParameter(void)
	{
		if (HpDraw != HpCurrent)
			HpDraw += (HpCurrent - HpDraw) * 0.2f;
		return HpDraw / HpMax;
	}
	void SetIsGameOver(bool state) { IsGameOver = state; }
	bool GetIsGameOver(void) const { return IsGameOver; }
	void DoMove(float ratio);
	bool AttackCollide(float damage, XMFLOAT3 repel) override;
	void AttackCameraShake(int frames, float minRange, float maxRange);
private:
	void SetupTransitions(void) override;
private:
	std::unique_ptr<Player_Head> visualPre_Head;
	std::unique_ptr<Player_Body> visualPre_Body;
	Graphics& Gfx;
	Rgph::RenderGraph& Rg;
	CameraContainer* pCamera;
	std::unique_ptr<StateMachine<Player>> FSM;
	float HpDraw{ 0.0f };
	bool IsGameOver{ false };

// input related
	struct PlayerInputSnapshot
	{
		int padIndex = 0;

		float moveX = 0.0f;
		float moveZ = 0.0f;

		bool moveHeld = false;

		bool attack = false;

		void ClearOneShots() noexcept
		{
			attack = false;
		}
	};
public:
	const PlayerInputSnapshot& Input() const noexcept { return inputSnap; }

	void BindPad(int idx) noexcept { boundPadIndex = idx; }   // 0..3
	void UseKeyboardMouse() noexcept { boundPadIndex = -1; }
	int GetBoundPad() const noexcept { return boundPadIndex; }
	PlayerInputSnapshot inputSnap{};
	int boundPadIndex = -1;
private:
	void BuildInputSnapshot();
};

/*------------------------------------------------------------------------------
   Player_IdleState
------------------------------------------------------------------------------*/
class Player_IdleState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_IDLE]; }
private:
};

/*------------------------------------------------------------------------------
   Player_MoveState
------------------------------------------------------------------------------*/
class Player_MoveState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_MOVE]; }
private:
};

/*------------------------------------------------------------------------------
   Player_AttackState
------------------------------------------------------------------------------*/
class Player_AttackState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override;
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_ATTACK]; }
private:
};

/*------------------------------------------------------------------------------
   Player_HurtState
------------------------------------------------------------------------------*/
class Player_HurtState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override;
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_HURT]; }
private:
};

/*------------------------------------------------------------------------------
   Player_DeathState
------------------------------------------------------------------------------*/
class Player_DeathState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_DEATH]; }
private:
};