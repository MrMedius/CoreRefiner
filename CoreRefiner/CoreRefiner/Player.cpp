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
		character_Enemy_T,
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

	// PLAYER_IDLE → PLAYER_ATTACK
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_ATTACK], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsAttack;
		});

	// PLAYER_MOVE → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->Input().moveHeld;
		});

	// PLAYER_MOVE → PLAYER_ATTACK
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_ATTACK], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsAttack;
		});

	// PLAYER_ATTACK → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsAttack;
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
	// attack: MouseLeft || J || Pad X
	inputSnap.attack =
		input.MouseLeftTriggered() ||
		input.KeyTriggered(KK_J) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && input.GP_Triggered(boundPadIndex, Gamepad::GP_X));
}


/*------------------------------------------------------------------------------
   Player_IdleState
------------------------------------------------------------------------------*/
void Player_IdleState::OnEnter(Player* owner)
{
}

void Player_IdleState::Update(Player* owner,  float dt)
{
	// 遷移判断
	const auto& in = owner->Input();
	if (in.attack)	owner->SetIsAttack(true);

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();
}


/*------------------------------------------------------------------------------
   Player_MoveState関数
------------------------------------------------------------------------------*/
void Player_MoveState::OnEnter(Player* owner)
{
}

void Player_MoveState::Update(Player* owner, float dt)
{
	// 遷移判断
	const auto& in = owner->Input();
	if (in.attack)	owner->SetIsAttack(true);

	// 移動
	owner->DoMove(1.0f);

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();
}


/*------------------------------------------------------------------------------
   Player_AttackState関数
------------------------------------------------------------------------------*/
void Player_AttackState::OnEnter(Player* owner)
{
}

void Player_AttackState::OnExit(Player* owner)
{
}

void Player_AttackState::Update(Player* owner, float dt)
{
	owner->SetIsAttack(false);
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