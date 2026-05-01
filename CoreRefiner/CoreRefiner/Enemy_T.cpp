#include "Enemy_T.h"
#include "Math.h"
#include "XMath.h"
#include "Channels.h"

void Enemy_Red_0::Update(float dt)
{
	// •ÏX‘O‚ÌÀ•W‚ðŠi”[
	auto Position = transInfo.position;
	PositionOld = Position;
	searchCollider.center = Position;
	CheckIsInArea();

	// UŒ‚”ÍˆÍ‚ÌXV
	if(IsInArea)
	{
		CheckIsInAttackArea();
	}

	// ó‘Ô‘JˆÚ
	FSM->Update(dt);

	// d—Í
	if (!OnFloor) MoveVelocity.y -= GRAVITY * dt;

	// ’ïR—Í
	MoveVelocity.x -= MoveVelocity.x * FORCE_RATE;
	MoveVelocity.z -= MoveVelocity.z * FORCE_RATE;

	// ˆÚ“®
	Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
	visualPre->SetPosition(transInfo.position);

	// ƒ}ƒbƒv—v‘f‚Æ‚Ì“–‚½‚è”»’è
	boxCollider.center = transInfo.position;
	MapItemCollide();
}

void Enemy_Red_0::Submit(void)
{
	// •`‰æ
	visualPre->Submit(Chan::main);
	visualPre->Submit(Chan::shadow);

#ifdef _DEBUG
	if (!IsDeath)
	{
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
		searchColliderWire->DoSubmit(transInfo.position, searchCollider.GetSize());
	}
#endif
}

void Enemy_Red_0::SetupTransitions(void)
{
	// ó‘Ô‚Ì‘JˆÚðŒ‚ð‘‰Á‚·‚é
	// ENEMY_IDLE ¨ ENEMY_CHASE
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_CHASE], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsInArea && !enemyRed0->IsInAttackArea; // ”»’f‚ÌðŒ
		});

	// ENEMY_IDLE ¨ ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsAttack;
		});

	// ENEMY_IDLE ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsHurt;
		});

	// ENEMY_CHASE ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return (!enemyRed0->IsInArea) || (enemyRed0->IsInAttackArea && enemyRed0->AttackCountDown > 0);
		});

	// ENEMY_CHASE ¨ ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsAttack;
		});

	// ENEMY_CHASE ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsHurt;
		});

	// ENEMY_ATTACK ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return !enemyRed0->IsAttack;
		});

	// ENEMY_ATTACK ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsHurt;
		});

	// ENEMY_HURT ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return !enemyRed0->IsHurt && !enemyRed0->IsDeath;
		});

	// ENEMY_HURT ¨ ENEMY_DEATH
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_DEATH], [](Character* owner) {
		auto enemyRed0 = static_cast<Enemy_Red_0*>(owner);
		return enemyRed0->IsDeath;
		});
}


/*------------------------------------------------------------------------------
   Enemy_Red_0_IdleState
------------------------------------------------------------------------------*/
void Enemy_Red_0_IdleState::OnEnter(Enemy_Red_0* owner)
{
	// set anime
}

void Enemy_Red_0_IdleState::Update(Enemy_Red_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);
	// general jobs
	owner->CheckIsAttack();		// UŒ‚‚·‚é‚©‚Ç‚¤‚©‚ð”»’f
	owner->DoAttackCountDown();	// UŒ‚‚ðƒJƒEƒ“ƒgƒ_ƒEƒ“
}


/*------------------------------------------------------------------------------
   Enemy_Red_0_ChaseState
------------------------------------------------------------------------------*/
void Enemy_Red_0_ChaseState::OnEnter(Enemy_Red_0* owner)
{
	// set anime
}

void Enemy_Red_0_ChaseState::Update(Enemy_Red_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);
	// general jobs
	owner->CheckIsAttack();		// UŒ‚‚·‚é‚©‚Ç‚¤‚©‚ð”»’f
	owner->DoChase();			// ƒ^[ƒQƒbƒg‚ð’Ç‚¢‚©‚¯‚é
	owner->DoAttackCountDown();	// UŒ‚‚ðƒJƒEƒ“ƒgƒ_ƒEƒ“
}


/*------------------------------------------------------------------------------
   Enemy_Red_0_AttackState
------------------------------------------------------------------------------*/
void Enemy_Red_0_AttackState::OnEnter(Enemy_Red_0* owner)
{
	// set anime
	FrameNoOld = 0;
	// make sure no repeated hits
	HaveHit = false;
	// set attack dash direction
	XMFLOAT3 pVec = owner->GetPosition();
	XMFLOAT3 tVec = owner->GetAttackTarget()->GetPosition();
	AttackVec = { tVec.x - pVec.x,0.0f,tVec.z - pVec.z };
	NormalizeXZ(AttackVec);
	// set collsiosns
	owner->SetAttackCollisionSize({ 2.0f, 2.0f, 2.0f });
}

void Enemy_Red_0_AttackState::OnExit(Enemy_Red_0* owner)
{
	owner->ResetAttackCountDown();			// UŒ‚I‚í‚Á‚½‚çUŒ‚‚ÌƒJƒEƒ“ƒ^[‚ðƒŠƒZƒbƒg‚·‚é
	owner->SetAttackCollisionOnOff(false);	// ‚à‚µUŒ‚‚Ì“r’†‚ÅUŒ‚‚³‚ê‚½‚çA‚±‚Á‚¿‚ÉUŒ‚ƒtƒ‰ƒO‚ÆƒRƒŠƒWƒ‡ƒ“‚ð•Â‚¶‚é
	owner->SetIsAttack(false);
	owner->SetAttackCollisionSize({ 10.0f,2.0f,5.0f });
}

void Enemy_Red_0_AttackState::Update(Enemy_Red_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	int currentFrame = owner->GetVisualPre()->GetCurrentFrame();
	// UŒ‚ƒRƒŠƒWƒ‡ƒ“‚ðŠJ‚¯‚éƒtƒŒ[ƒ€
	if (currentFrame == 8 && FrameNoOld != currentFrame)
	{
		owner->SetAttackCollisionOnOff(true);
		SoundCodex::Get().PlaySE(SndPath::SE_Enemy_Attack_Red_0);
	}
	// UŒ‚ƒRƒŠƒWƒ‡ƒ“‚ð•Â‚¶‚éƒtƒŒ[ƒ€
	if (currentFrame == 20 && FrameNoOld != currentFrame) owner->SetAttackCollisionOnOff(false);

	// dash attack
	if (currentFrame < 20 && FrameNoOld != currentFrame)
	{
		float a = 20.0f;
		float accel = ease_in_back<float>(currentFrame / a, 7 / a, 1.5f) * 0.1f;
		owner->CalculateMoveVelocity(AttackVec.x * accel, 0.005f, AttackVec.z * accel);
	}
	
	// UŒ‚“–‚½‚é”»’è
	if (owner->GetAttackCollisionOnOff() && !HaveHit) HaveHit = owner->AttackCollide(-1.0f, { 0.5f ,0.1f,0.5f });

	// I‚í‚Á‚½‚çUŒ‚ƒtƒ‰ƒO‚ð•Â‚¶‚é
	if (owner->GetVisualPre()->ClipFinished()) owner->SetIsAttack(false);

	FrameNoOld = currentFrame;
}


/*------------------------------------------------------------------------------
   Enemy_Red_0_HurtState
------------------------------------------------------------------------------*/
void Enemy_Red_0_HurtState::OnEnter(Enemy_Red_0* owner)
{
	// set anime
	// make sure not be hit repeatly
}

void Enemy_Red_0_HurtState::OnExit(Enemy_Red_0* owner)
{
	owner->SetIsHurt(false);
}

void Enemy_Red_0_HurtState::Update(Enemy_Red_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);


	// UŒ‚‚ðƒJƒEƒ“ƒgƒ_ƒEƒ“
	owner->DoAttackCountDown();

	// UŒ‚‚³‚ê‚½Œã‚Í‰½‚·‚é‚Ì”»’f
	if (owner->GetVisualPre()->ClipFinished())
	{
		// ‚à‚µ‘Ì—Í‚ªƒ[ƒ‚½‚çAŽ€‚Êó‘Ô‚Å‘JˆÚ
		if (owner->GetHpCurrent() == 0)	owner->SetIsDeath(true);
		// I‚í‚Á‚½‚çUŒ‚‚³‚ê‚½ƒtƒ‰ƒO‚ð•Â‚¶‚é
		else owner->SetIsHurt(false);
	}
}


/*------------------------------------------------------------------------------
   Enemy_Red_0_DeathState
------------------------------------------------------------------------------*/
void Enemy_Red_0_DeathState::OnEnter(Enemy_Red_0* owner)
{
	// set anime
	SoundCodex::Get().PlaySE(SndPath::SE_Enemy_Dead_Red_0);
}

void Enemy_Red_0_DeathState::Update(Enemy_Red_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	// I‚í‚Á‚½‚çŽg—pI‚í‚é‚ðƒZƒbƒg‚·‚é
	if (owner->GetVisualPre()->ClipFinished()) owner->Deactivate();
}