#pragma once
#include "Character.h"
#include "RenderGraph.h"
#include "ObjectCodex.h"
#include "CameraContainer.h"
#include "Player_Shape.h"
#include "VisualComponent.h"
#include "ColliderComponent.h"
#include "Channels.h"

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
		SetHpMax(30.0f);
		ResetHpCurrent();
		SetMoveAccel(0.035f);
		SetAttackInterval(1.5f);

		// graphics init — VisualComponent owns each Drawable (shape scale independent; syncScale=false)
		{
			auto head = std::make_unique<Player_Head>(gfx, XMFLOAT3{ 3.0f, 3.0f, 3.0f });
			head->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(head), Chan::main, false, false);
		}
		{
			auto body = std::make_unique<Player_Body>(gfx, XMFLOAT3{ 3.0f, 3.0f, 3.0f });
			body->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(body), Chan::main, false, false);
		}

		pCollider_ = AddComponent<ColliderComponent>(
			Collider3D::CollideType::Capsule,
			ColliderSyncMode::FollowCenter);
		// Was Box full size {1.5, 5, 2} → radius 1, total height 5
		pCollider_->SetCapsule(1.0f, 5.0f);
		// World-space offset from Player::GetPosition() to capsule center (tune Y if pivot ≠ waist)
		pCollider_->SetCenterOffset({ 0.0f, 1.0f, 0.0f });
		pCollider_->SetEnabled(true);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3{ 0.0f, 1.0f, 1.0f }, "wirePlayerCapsule");

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
protected:
	/**
	 * @brief Player collision resolve via Capsule + TrySeparate (not Character AABB).
	 */
	void MapItemCollide(void) override;
private:
	void SetupTransitions(void) override;
private:
	ColliderComponent* pCollider_{ nullptr };
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