#include "Player.h"
#include "Enemy.h"
#include "Environment.h"
#include "ObjectCodex.h"
#include "Collision3D.h"
#include "ColliderComponentBase.h"
#include "CapsuleColliderComponent.h"

#include "InputCodex.h"
#include "SoundCodex.h"
#include "GameStatsCodex.h"
#include "XMath.h"

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
	auto Position = GetPosition();
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
	}

	if (pCollider_ != nullptr)
	{
		pCollider_->SyncFromOwner();
	}
	MapItemCollide();

	ObjectBase::Update(dt);
}

/**
 * @brief Capsule resolve via contact normal from TrySeparate (floor stick + skin).
 */
void Player::MapItemCollide(void)
{
	auto* selfCol = pCollider_ != nullptr ? pCollider_ : GetComponent<ColliderComponentBase>();
	if (selfCol == nullptr || !selfCol->IsEnabled())
	{
		return;
	}

	using namespace PlayerCapsuleTuning;

	/**
	 * @brief Apply depenetration along contact normal; floor contacts clear downward speed.
	 * @param n Unit contact normal (points out of the other body toward this capsule).
	 * @param depth Penetration along n (>= 0). Exact touch (0) still gets skin.
	 * @return true when this contact is a floor (supports OnFloor).
	 */
	auto applyContact = [this, selfCol](DirectX::XMFLOAT3 n, float depth) -> bool
	{
		const bool isFloor = n.y > kFloorNormalY;
		// Contact-based: always keep a skin gap along the contact normal (including depth == 0).
		const float push = (std::max)(depth, 0.0f) + kSkin;

		SetPosition(V(GetPosition()) + V(n) * push);
		selfCol->SyncFromOwner();

		if (isFloor)
		{
			if (MoveVelocity.y < 0.0f)
			{
				MoveVelocity.y = 0.0f;
			}
		}
		else
		{
			const float vn = Dot(V(MoveVelocity), V(n));
			if (vn < 0.0f)
			{
				MoveVelocity = (V(MoveVelocity) - V(n) * vn).ToFloat3();
			}
		}
		return isFloor;
	};

	bool grounded = false;

	/*------------------------------------------------------------------------------
	   MapEnvironment — contact normal from Capsule↔Box separation
	------------------------------------------------------------------------------*/
	std::vector<Environment*> mapEnvironment;
	for (auto tag : { environment_Field })
	{
		auto found = ObjectCodex::FindActiveObjectsByTag<Environment>(tag);
		mapEnvironment.reserve(mapEnvironment.size() + found.size());
		mapEnvironment.insert(mapEnvironment.end(), found.begin(), found.end());
	}

	for (auto* e : mapEnvironment)
	{
		auto* eCol = e->GetComponent<ColliderComponentBase>();
		if (eCol == nullptr || !eCol->IsEnabled())
		{
			continue;
		}
		if (!CollisionSystem::IsOverlap(selfCol->GetVolume(), eCol->GetVolume()))
		{
			continue;
		}

		e->OnCollide(this);

		DirectX::XMFLOAT3 n{};
		float depth = 0.0f;
		if (CollisionSystem::TrySeparate(selfCol->GetVolume(), eCol->GetVolume(), n, depth))
		{
			if (applyContact(n, depth))
			{
				grounded = true;
			}
		}
	}

	// Floor probe: after skin we may leave overlap; step down to re-acquire floor contact.
	if (!grounded)
	{
		auto p = GetPosition();
		const float yRestore = p.y;
		p.y -= kFloorProbe;
		SetPosition(p);
		selfCol->SyncFromOwner();

		bool probeHit = false;
		for (auto* e : mapEnvironment)
		{
			auto* eCol = e->GetComponent<ColliderComponentBase>();
			if (eCol == nullptr || !eCol->IsEnabled())
			{
				continue;
			}
			if (!CollisionSystem::IsOverlap(selfCol->GetVolume(), eCol->GetVolume()))
			{
				continue;
			}

			DirectX::XMFLOAT3 n{};
			float depth = 0.0f;
			if (CollisionSystem::TrySeparate(selfCol->GetVolume(), eCol->GetVolume(), n, depth) &&
				n.y > kFloorNormalY)
			{
				applyContact(n, depth);
				grounded = true;
				probeHit = true;
				break;
			}
		}

		if (!probeHit)
		{
			p.y = yRestore;
			SetPosition(p);
			selfCol->SyncFromOwner();
		}
	}

	OnFloor = grounded;

	/*------------------------------------------------------------------------------
	   MapCharacter
	------------------------------------------------------------------------------*/
	std::vector<Character*> mapCharacters;
	for (auto tag : { character_Player, character_Enemy_T })
	{
		auto found = ObjectCodex::FindActiveObjectsByTag<Character>(tag);
		mapCharacters.reserve(mapCharacters.size() + found.size());
		mapCharacters.insert(mapCharacters.end(), found.begin(), found.end());
	}

	for (auto* c : mapCharacters)
	{
		auto* cCol = c->GetComponent<ColliderComponentBase>();
		if (this == c || this->GetIsDeath() || c->GetIsDeath() ||
			cCol == nullptr || !cCol->IsEnabled())
		{
			continue;
		}
		if (!CollisionSystem::IsOverlap(selfCol->GetVolume(), cCol->GetVolume()))
		{
			continue;
		}

		DirectX::XMFLOAT3 n{};
		float depth = 0.0f;
		if (CollisionSystem::TrySeparate(selfCol->GetVolume(), cCol->GetVolume(), n, depth))
		{
			if (applyContact(n, depth))
			{
				OnFloor = true;
			}
		}
	}
}

void Player::Submit(void)
{
	if (pCollider_ != nullptr)
	{
		pCollider_->SetDebugDraw(!IsDeath);
	}
	ObjectBase::Submit();
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
   Player_MoveState
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
   Player_AttackState
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
	if (auto* col = owner->GetComponent<ColliderComponentBase>())
	{
		col->SetEnabled(false); // コリジュンを閉じる
	}

	SoundCodex::Get().PlaySE(SndPath::SE_Player_Hurt);
	InputCodex::Get().GP_SetVibrationPulse(owner->boundPadIndex, 0.8f, 0.8f, 30);
}

void Player_HurtState::OnExit(Player* owner)
{
	owner->SetIsHurt(false); // DeathStateに遷移するかも、も一回IsHurtをリセットする
	if (auto* col = owner->GetComponent<ColliderComponentBase>())
	{
		col->SetEnabled(true); // コリジュンを開ける
	}
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