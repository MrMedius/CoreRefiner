#include "Enemy_Red_T.h"
#include "Math.h"
#include "XMath.h"
#include "Channels.h"

void Enemy_Red_T::Update(float dt)
{
	// ïœçXëOÇÃç¿ïWÇäiî[
	auto Position = transInfo.position;
	PositionOld = Position;

	// èÛë‘ëJà⁄
	FSM->Update(dt);

	// èdóÕ
	if (!OnFloor) MoveVelocity.y -= GRAVITY * dt;

	// íÔçRóÕ
	MoveVelocity.x -= MoveVelocity.x * FORCE_RATE;
	MoveVelocity.z -= MoveVelocity.z * FORCE_RATE;

	// à⁄ìÆ
	Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
	visualPre->SetPosition(transInfo.position);

	// âÒì]
	waveZ += MoveAccel * (IsFlip ? 1.0f : -1.0f);
	if (waveZ >= waveLimit)
	{
		waveZ = waveLimit;
		IsFlip = false;
	}
	else if (waveZ <= -waveLimit)
	{
		waveZ = -waveLimit;
		IsFlip = true;
	}
	visualPre->SetRotation({ waveX, 0.0f, waveZ });

	// É}ÉbÉvóvëfÇ∆ÇÃìñÇΩÇËîªíË
	boxCollider.center = transInfo.position;
	MapItemCollide();
}

void Enemy_Red_T::Submit(void)
{
	// ï`âÊ
	visualPre->Submit(Chan::main);
	visualPre->Submit(Chan::shadow);

#ifdef _DEBUG
	if (!IsDeath)
	{
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
	}
#endif
}

void Enemy_Red_T::SetupTransitions(void)
{
	// èÛë‘ÇÃëJà⁄èåèÇëùâ¡Ç∑ÇÈ
	// ENEMY_IDLE Å® ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyRedT = static_cast<Enemy_Red_T*>(owner);
		return enemyRedT->IsHurt;
		});

	// ENEMY_IDLE Å® ENEMY_DEATH
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_DEATH], [](Character* owner) {
		auto enemyRedT = static_cast<Enemy_Red_T*>(owner);
		return enemyRedT->IsDeath;
		});

	// ENEMY_HURT Å® ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyRedT = static_cast<Enemy_Red_T*>(owner);
		return !enemyRedT->IsHurt && !enemyRedT->IsDeath;
		});

	// ENEMY_HURT Å® ENEMY_DEATH
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_DEATH], [](Character* owner) {
		auto enemyRedT = static_cast<Enemy_Red_T*>(owner);
		return enemyRedT->IsDeath;
		});
}


/*------------------------------------------------------------------------------
   Enemy_Red_T_IdleState
------------------------------------------------------------------------------*/
void Enemy_Red_T_IdleState::OnEnter(Enemy_Red_T* owner)
{
	// set anime
	owner->SetMoveAccel(0.5f);
	owner->SetWaveLimit(10.0f);
}

/*------------------------------------------------------------------------------
   Enemy_Red_T_HurtState
------------------------------------------------------------------------------*/
void Enemy_Red_T_HurtState::OnEnter(Enemy_Red_T* owner)
{
	// set anime
	owner->SetMoveAccel(3.0f);
	owner->SetWaveLimit(30.0f);
	// make sure not be hit repeatly
	BeAttackedTypeOld = owner->GetBeAttackedType();
}

void Enemy_Red_T_HurtState::OnExit(Enemy_Red_T* owner)
{
	owner->SetIsHurt(false);
	owner->SetBeAttackedType(Attack_Type_None);
}

void Enemy_Red_T_HurtState::Update(Enemy_Red_T* owner, float dt)
{
	if (owner->GetBeAttackedType() != BeAttackedTypeOld) BeAttackedTypeOld = owner->GetBeAttackedType();

	float wave = owner->GetWaveLimit();
	wave -= dt * 10.0f;
	owner->SetWaveLimit(wave);
	owner->SetMoveAccel(wave / 10.0f);

	// çUåÇÇ≥ÇÍÇΩå„ÇÕâΩÇ∑ÇÈÇÃîªíf
	if (wave <= 10.0f)
	{
		// Ç‡ÇµëÃóÕÇ™É[ÉçÇΩÇÁÅAéÄÇ èÛë‘Ç≈ëJà⁄
		if (owner->GetHpCurrent() == 0)	owner->SetIsDeath(true);
		// èIÇÌÇ¡ÇΩÇÁçUåÇÇ≥ÇÍÇΩÉtÉâÉOÇï¬Ç∂ÇÈ
		else owner->SetIsHurt(false);
	}
}


/*------------------------------------------------------------------------------
   Enemy_Red_T_DeathState
------------------------------------------------------------------------------*/
void Enemy_Red_T_DeathState::OnEnter(Enemy_Red_T* owner)
{
	// set anime
	owner->SetWaveLimit(0.0f);
}

void Enemy_Red_T_DeathState::Update(Enemy_Red_T* owner, float dt)
{
	// set anime
	owner->AddWaveX(2.0f);

	// èIÇÌÇ¡ÇΩÇÁégópèIÇÌÇÈÇÉZÉbÉgÇ∑ÇÈ
	if (owner->GetWaveX() > 90.0f) owner->Deactivate();
}