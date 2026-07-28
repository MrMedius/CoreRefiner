#include "Enemy_T.h"
#include "Math.h"
#include "XMath.h"

void Enemy_T::Update(float dt)
{
	// 変更前の座標を格納
	auto Position = GetPosition();
	PositionOld = Position;
	searchCollider.center = Position;
	CheckIsInArea();

	// 状態遷移
	FSM->Update(dt);

	// 重力
	if (!OnFloor) MoveVelocity.y -= GRAVITY * dt;

	// 抵抗力
	MoveVelocity.x -= MoveVelocity.x * FORCE_RATE;
	MoveVelocity.z -= MoveVelocity.z * FORCE_RATE;

	// 移動
	Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);

	// collider must be current before MapItemCollide (full component drive runs after resolve)
	if (pCollider_ != nullptr)
	{
		pCollider_->SyncFromOwner();
	}
	// Enemy intentionally keeps Character::MapItemCollide (Box AABB env + character push).
	MapItemCollide();

	// host-driven: sync Visual/Collider to post-collision transform
	ObjectBase::Update(dt);
}

void Enemy_T::Submit(void)
{
	if (pCollider_ != nullptr)
	{
		pCollider_->SetDebugDraw(!IsDeath);
	}
	ObjectBase::Submit();
}

void Enemy_T::SetupTransitions(void)
{
	// 状態の遷移条件を増加する
	// ENEMY_CHASE → ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemy = static_cast<Enemy_T*>(owner);
		return enemy->IsAttack;
		});

	// ENEMY_CHASE → ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemy = static_cast<Enemy_T*>(owner);
		return enemy->IsHurt;
		});

	// ENEMY_ATTACK → ENEMY_CHASE
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_CHASE], [](Character* owner) {
		auto enemy = static_cast<Enemy_T*>(owner);
		return !enemy->IsAttack;
		});

	// ENEMY_ATTACK → ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemy = static_cast<Enemy_T*>(owner);
		return enemy->IsHurt;
		});

	// ENEMY_HURT → ENEMY_CHASE
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_CHASE], [](Character* owner) {
		auto enemy = static_cast<Enemy_T*>(owner);
		return !enemy->IsHurt && !enemy->IsDeath;
		});

	// ENEMY_HURT → ENEMY_DEATH
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_DEATH], [](Character* owner) {
		auto enemy = static_cast<Enemy_T*>(owner);
		return enemy->IsDeath;
		});
}


/*------------------------------------------------------------------------------
   Enemy_T_ChaseState
------------------------------------------------------------------------------*/
void Enemy_T_ChaseState::OnEnter(Enemy_T* owner)
{
	// set anime
}

void Enemy_T_ChaseState::Update(Enemy_T* owner, float dt)
{
	// general jobs
	owner->DoChase();			// ターゲットを追いかける
}


/*------------------------------------------------------------------------------
   Enemy_T_AttackState
------------------------------------------------------------------------------*/
void Enemy_T_AttackState::OnEnter(Enemy_T* owner)
{
}

void Enemy_T_AttackState::OnExit(Enemy_T* owner)
{
}

void Enemy_T_AttackState::Update(Enemy_T* owner, float dt)
{
}


/*------------------------------------------------------------------------------
   Enemy_T_HurtState
------------------------------------------------------------------------------*/
void Enemy_T_HurtState::OnEnter(Enemy_T* owner)
{
	// set anime
	// make sure not be hit repeatly
}

void Enemy_T_HurtState::OnExit(Enemy_T* owner)
{
}

void Enemy_T_HurtState::Update(Enemy_T* owner, float dt)
{
	// set anime
	if (auto* visual = owner->GetVisual())
	{
		visual->Update(dt);
	}

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();

	if (owner->GetHpCurrent() == 0)	owner->SetIsDeath(true);
	else owner->SetIsHurt(false);
}


/*------------------------------------------------------------------------------
   Enemy_T_DeathState
------------------------------------------------------------------------------*/
void Enemy_T_DeathState::OnEnter(Enemy_T* owner)
{
	// set anime
	SoundCodex::Get().PlaySE(SndPath::SE_Enemy_Dead_Red_0);
}

void Enemy_T_DeathState::Update(Enemy_T* owner, float dt)
{
	// set anime
	if (auto* visual = owner->GetVisual())
	{
		visual->Update(dt);
	}

	// 終わったら使用終わるをセットする（帧末 Flush 再 Deactivate）
	owner->RequestDisable();
}