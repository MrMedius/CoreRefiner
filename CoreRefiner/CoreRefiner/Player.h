#pragma once
#include "Character.h"
#include "RenderGraph.h"
#include "ObjectCodex.h"
#include "CameraContainer.h"
#include "Player_Shape.h"
#include "VisualComponent.h"
#include "CapsuleColliderComponent.h"
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

/**
 * @brief Player Capsule feel / debug knobs (tune here only; MapItemCollide reads the same values).
 * @note Height 5 沿用旧 Box；半径按身体视觉半宽略放大，让贴身与判定更接近。
 */
namespace PlayerCapsuleTuning
{
	/** @brief 接近身体视觉半径（Pyramid base 0.5 × size 3 ≈ 1.5）。 */
	constexpr float kRadius = 1.25f;
	constexpr float kTotalHeight = 5.0f;
	/** @brief World Y offset from Player::GetPosition() to capsule center. */
	constexpr float kCenterOffsetY = 1.0f;
	/** @brief Extra separation along contact normal (incl. exact touch) to reduce jitter. */
	constexpr float kSkin = 0.02f;
	/** @brief Contact normal.y above this counts as floor support. */
	constexpr float kFloorNormalY = 0.5f;
	/** @brief Downward probe distance to re-acquire floor after skin push. */
	constexpr float kFloorProbe = 0.08f;
	/** @brief Cyan debug wire — contrasts Enemy Box red / Ball Sphere green. */
	inline constexpr float kDebugWireR = 0.0f;
	inline constexpr float kDebugWireG = 1.0f;
	inline constexpr float kDebugWireB = 1.0f;
	inline constexpr const char* kDebugWireName = "wirePlayerCapsule";
}

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
		SetSize({ 3.0f, 3.0f, 3.0f });
		SetHpMax(30.0f);
		ResetHpCurrent();
		SetMoveAccel(0.035f);
		SetAttackInterval(1.5f);

		// graphics init — VisualComponent owns each Drawable (shape scale independent; syncScale=false)
		{
			auto head = std::make_unique<Player_Head>(gfx, GetSize());
			head->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(head), Chan::main, false, false);
		}
		{
			auto body = std::make_unique<Player_Body>(gfx, GetSize());
			body->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(body), Chan::main, false, false);
		}

		pCollider_ = AddComponent<CapsuleColliderComponent>(
			PlayerCapsuleTuning::kRadius,
			PlayerCapsuleTuning::kTotalHeight,
			ColliderSyncMode::FollowCenter);
		pCollider_->SetCenterOffset({ 0.0f, PlayerCapsuleTuning::kCenterOffsetY, 0.0f });
		pCollider_->SetEnabled(true);
		pCollider_->LinkDebugWire(
			gfx,
			rg,
			XMFLOAT3{
				PlayerCapsuleTuning::kDebugWireR,
				PlayerCapsuleTuning::kDebugWireG,
				PlayerCapsuleTuning::kDebugWireB },
			PlayerCapsuleTuning::kDebugWireName);

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
		SetPosition(XMFLOAT3(0.0f, 5.0f, 0.0f));
		MoveVelocity = { 0.0f,0.0f,0.0f };
		ResetHpCurrent();
		HpDraw = HpCurrent;
		SetIsHurt(false);
		SetIsDeath(false);
		SetIsGameOver(false);
		EndHurtIFrames();
		if (pCollider_ != nullptr)
		{
			pCollider_->SetEnabled(true);
		}
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
	/** @brief Hurt 入场：重置无敌计时与闪烁。 */
	void BeginHurtIFrames();
	/** @brief Hurt 每帧：闪烁、HP 归零则死亡，否则到时解除 Hurt。 */
	void TickHurtIFrames(float dt);
	/** @brief Hurt 离场：关掉闪烁隐藏。 */
	void EndHurtIFrames();
	[[nodiscard]] bool IsHurtFlashHidden() const noexcept { return hurtFlashHide_; }
protected:
	/**
	 * @brief Player collision resolve via Capsule + TrySeparate (not Character AABB).
	 */
	void MapItemCollide(void) override;
private:
	void SetupTransitions(void) override;
private:
	CapsuleColliderComponent* pCollider_{ nullptr };
	Graphics& Gfx;
	Rgph::RenderGraph& Rg;
	CameraContainer* pCamera;
	std::unique_ptr<StateMachine<Player>> FSM;
	float HpDraw{ 0.0f };
	bool IsGameOver{ false };
	float hurtElapsed_{ 0.0f };
	bool hurtFlashHide_{ false };
	static constexpr float kHurtIFrameSec_ = 0.55f;
	static constexpr float kHurtFlashPeriod_ = 0.07f;

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