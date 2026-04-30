#include "Player.h"
#include "Enemy.h"
#include "Channels.h"

#include "InputCodex.h"
#include "SoundCodex.h"
#include "GameStatsCodex.h"

static void Normalize2D(float& x, float& z) noexcept
{
	float len2 = x * x + z * z;
	if (len2 <= 0.000001f) return;
	float invLen = 1.0f / sqrtf(len2);
	x *= invLen;
	z *= invLen;
}

void Player::Update(float dt)
{
	BuildInputSnapshot();

	// 変更前の座標を格納
	auto Position = transInfo.position;
	PositionOld = Position;
	// 攻撃範囲の更新
	if (!IsFlip) attackCollider.center = { Position.x + boxCollider.half.x + attackCollider.half.x, Position.y, Position.z };
	else attackCollider.center = { Position.x - boxCollider.half.x - attackCollider.half.x, Position.y, Position.z };

	// 状態遷移
	// FSM update
	FSM->Update(dt);

	// 重力
	if (!OnFloor) 
		MoveVelocity.y -= GRAVITY * dt;

	// 抵抗力
	MoveVelocity.x -= MoveVelocity.x * FORCE_RATE;
	MoveVelocity.z -= MoveVelocity.z * FORCE_RATE;

	// 移動
	{
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		visualPre_Head->SetPosition(transInfo.position);
		visualPre_Body->SetPosition(transInfo.position);
	}

	// マップ要素との当たり判定
	boxCollider.center = transInfo.position;
	MapItemCollide();
}

void Player::Submit(void)
{
	// 描画
	// visualPre
	visualPre_Head->Submit(Chan::main);
	visualPre_Body->Submit(Chan::main);

#ifdef _DEBUG
	if (!IsDeath)
	{
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
		attackColliderWire->DoSubmit(attackCollider.center, attackCollider.GetSize());
	}
#endif
}

void Player::DoMove(float ratio)
{
	if (inputSnap.moveHeld)
	{
		float dx = inputSnap.moveX;
		float dz = inputSnap.moveZ;

		float angle = atan2f(dx, dz);
		CalculateMoveVelocity(sinf(angle) * GetMoveAccel() * ratio, 0.0f, cosf(angle) * GetMoveAccel() * ratio);
	}
}

bool Player::AttackCollide(float damage, XMFLOAT3 repel)
{
	std::vector<Enemy*> enemies;
	for (auto tag : {
		character_Enemy_Red_0,
		}) {
		// プレイヤーを攻撃できる全ての敵を探す
		auto found = ObjectCodex::FindActiveObjectsByTag<Enemy>(tag);
		// 全ての敵を整理する
		enemies.reserve(enemies.size() + found.size());
		enemies.insert(enemies.end(), found.begin(), found.end());
	}

	return false;
}

void Player::AttackCameraShake(int frames, float minRange, float maxRange)
{
	pCamera->SetScreenShake(frames, minRange, maxRange);
}

void Player::SetupTransitions(void)
{
	// 状態の遷移条件を増加する
	// PLAYER_IDLE → PLAYER_MOVE
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_MOVE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);	// 入った対象をPlayerになる
		return player->Input().moveHeld;
		});

	// PLAYER_IDLE → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsDash && player->OnFloor;
		});

	// PLAYER_IDLE → PLAYER_ATTACK
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_ATTACK], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsAttack;
		});

	// PLAYER_IDLE → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsHurt;
		});

	// PLAYER_MOVE → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->Input().moveHeld;
		});

	// PLAYER_MOVE → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsDash && player->OnFloor;
		});

	// PLAYER_MOVE → PLAYER_ATTACK
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_ATTACK], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsAttack;
		});

	// PLAYER_MOVE → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsHurt;
		});

	// PLAYER_DASH → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_DASH], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsDash;
		});

	// PLAYER_DASH → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_DASH], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsHurt;
		});

	// PLAYER_ATTACK → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsAttack;
		});

	// PLAYER_ATTACK → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsDash;
		});

	// PLAYER_ATTACK → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsHurt;
		});

	// PLAYER_HURT → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_HURT], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsHurt && !player->IsDeath;
		});

	// PLAYER_HURT → PLAYER_DEATH
	FSM->AddTransition(PLAYER_STATE[PLAYER_HURT], PLAYER_STATE[PLAYER_DEATH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsDeath;
		});
}

void Player::BuildInputSnapshot()
{
	auto& input = InputCodex::Get();

	inputSnap.ClearOneShots();
	inputSnap.padIndex = boundPadIndex;

	// -----------------------
	// Move
	float mx = 0.0f;
	float mz = 0.0f;
	// keyboard WASD
	if (input.KeyPressed(KK_A)) mx -= 1.0f;
	if (input.KeyPressed(KK_D)) mx += 1.0f;
	if (input.KeyPressed(KK_W)) mz += 1.0f;
	if (input.KeyPressed(KK_S)) mz -= 1.0f;
	// gamepad left stick
	if (boundPadIndex >= 0 && input.PadConnected(boundPadIndex))
	{
		mx += input.GP_LeftX(boundPadIndex);
		mz += input.GP_LeftY(boundPadIndex);
	}
	// gamepad cross key
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_LEFT))  mx -= 1.0f;
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_RIGHT)) mx += 1.0f;
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_UP))	 mz += 1.0f;
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_DOWN))  mz -= 1.0f;
	// deadzone
	const float moveEps = 0.15f;
	if (fabsf(mx) < moveEps) mx = 0.0f;
	if (fabsf(mz) < moveEps) mz = 0.0f;
	Normalize2D(mx, mz);
	inputSnap.moveX = mx;
	inputSnap.moveZ = mz;
	inputSnap.moveHeld = (mx != 0.0f || mz != 0.0f);

	// -----------------------
	// Dash / Attack / Skill / SlotL / SlotR
	// dash: Space || Shift || Pad Y & B
	inputSnap.dash =
		input.KeyTriggered(KK_SPACE) || input.KeyTriggered(KK_LEFTSHIFT) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && (input.GP_Triggered(boundPadIndex, Gamepad::GP_Y) || input.GP_Triggered(boundPadIndex, Gamepad::GP_B)));
	// attack: MouseLeft || J || Pad X
	inputSnap.attack =
		input.MouseLeftTriggered() ||
		input.KeyTriggered(KK_J) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && input.GP_Triggered(boundPadIndex, Gamepad::GP_X));
	// skill: MouseRight || K || Pad A
	inputSnap.skill =
		input.MouseRightTriggered() ||
		input.KeyTriggered(KK_K) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && input.GP_Triggered(boundPadIndex, Gamepad::GP_A));
	// slotL: Q || Pad LB & LT
	inputSnap.slotL =
		input.KeyTriggered(KK_Q) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && (input.GP_Triggered(boundPadIndex, Gamepad::GP_LB) || input.GP_LT_Triggered(boundPadIndex)));
	// slotR: E || Pad RB & RT
	inputSnap.slotR =
		input.KeyTriggered(KK_E) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && (input.GP_Triggered(boundPadIndex, Gamepad::GP_RB) || input.GP_RT_Triggered(boundPadIndex)));
}


/*------------------------------------------------------------------------------
   Player_IdleState
------------------------------------------------------------------------------*/
Player_IdleState::Player_IdleState()
{
	packs.emplace_back(12, 16, 145, 24);
	packs.emplace_back(12, 12,  49, 24);
	packs.emplace_back(12, 10,  97, 24);

	assist.Loop = true;
	assist.FPS = packs[0].FPS;
	assist.FrameTotal = packs[0].FrameTotalCount;
}

void Player_IdleState::OnEnter(Player* owner)
{
	assist.Reset();
}

void Player_IdleState::Update(Player* owner,  float dt)
{
	assist.Update(dt);

	// 遷移判断
	const auto& in = owner->Input();
	if (in.dash) owner->SetIsDash(true);
	if (in.attack)	owner->SetIsAttack(true);

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();
}


/*------------------------------------------------------------------------------
   Player_MoveState関数
------------------------------------------------------------------------------*/
Player_MoveState::Player_MoveState()
{
	packs.emplace_back(12, 16, 169, 24);
	packs.emplace_back(12, 12,  73, 24);
	packs.emplace_back(12, 10,  97, 24);

	assist.Loop = true;
	assist.FPS = packs[0].FPS;
	assist.FrameTotal = packs[0].FrameTotalCount;
}

void Player_MoveState::OnEnter(Player* owner)
{
	assist.Reset();
}

void Player_MoveState::Update(Player* owner, float dt)
{
	assist.Update(dt);

	// 遷移判断
	const auto& in = owner->Input();
	if (in.dash) owner->SetIsDash(true);
	if (in.attack)	owner->SetIsAttack(true);

	// 方向判断
	if (in.moveX < 0) owner->SetIsFlip(true);
	else owner->SetIsFlip(false);

	// 移動
	owner->DoMove(1.0f);

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();
}


/*------------------------------------------------------------------------------
   Player_DashState関数
------------------------------------------------------------------------------*/
void Player_DashState::OnEnter(Player* owner)
{
}

void Player_DashState::Update(Player* owner, float dt)
{
}


/*------------------------------------------------------------------------------
   Player_AttackState関数
------------------------------------------------------------------------------*/
Player_AttackState::Player_AttackState()
{
}

void Player_AttackState::OnEnter(Player* owner)
{
}

void Player_AttackState::OnExit(Player* owner)
{
}

void Player_AttackState::Update(Player* owner, float dt)
{
}


/*------------------------------------------------------------------------------
   Player_HurtState
------------------------------------------------------------------------------*/
void Player_HurtState::OnEnter(Player* owner)
{
	// anime set
	owner->SetCollisionOnOff(false); // コリジュンを閉じる

	SoundCodex::Get().PlaySE(SndPath::SE_Player_Hurt);
	InputCodex::Get().GP_SetVibrationPulse(owner->boundPadIndex, 0.8f, 0.8f, 30);
}

void Player_HurtState::OnExit(Player* owner)
{
	owner->SetIsHurt(false); // DeathStateに遷移するかも、も一回IsHurtをリセットする
	owner->SetCollisionOnOff(true); // コリジュンを開ける
}

void Player_HurtState::Update(Player* owner, float dt)
{
}


/*------------------------------------------------------------------------------
   Player_DeathState
------------------------------------------------------------------------------*/
void Player_DeathState::OnEnter(Player* owner)
{
	// anime set
}

void Player_DeathState::Update(Player* owner, float dt)
{
	// 終わったら何がする…
}