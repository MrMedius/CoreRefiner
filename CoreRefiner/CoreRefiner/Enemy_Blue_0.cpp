#include "Enemy_Blue_0.h"
#include "Channels.h"

void Enemy_Blue_0::Update(float dt)
{
	// •ÏX‘O‚ÌÀ•W‚ðŠi”[
	auto Position = transInfo.position;
	PositionOld = Position;
	searchCollider.center = Position;
	CheckIsInArea();

	// UŒ‚”ÍˆÍ‚ÌXV
	if(IsInArea)
	{
		attackCollider.center = Position;
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

void Enemy_Blue_0::Submit(void)
{
	// •`‰æ
	visualPre->Submit(Chan::main);
	visualPre->Submit(Chan::shadow);

#ifdef _DEBUG
	if (!IsDeath)
	{
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
		searchColliderWire->DoSubmit(transInfo.position, searchCollider.GetSize());
		if (IsInArea) attackColliderWire->DoSubmit(attackCollider.center, attackCollider.GetSize());
	}
#endif
}

void Enemy_Blue_0::SetupTransitions(void)
{
	// ó‘Ô‚Ì‘JˆÚðŒ‚ð‘‰Á‚·‚é
	// ENEMY_IDLE ¨ ENEMY_CHASE
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_CHASE], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsInArea && !enemyBlue0->IsInAttackArea; // ”»’f‚ÌðŒ
		});

	// ENEMY_IDLE ¨ ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsAttack;
		});

	// ENEMY_IDLE ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsHurt;
		});

	// ENEMY_CHASE ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return (!enemyBlue0->IsInArea) || (enemyBlue0->IsInAttackArea && enemyBlue0->AttackCountDown > 0);
		});

	// ENEMY_CHASE ¨ ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsAttack;
		});

	// ENEMY_CHASE ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsHurt;
		});

	// ENEMY_ATTACK ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return !enemyBlue0->IsAttack;
		});

	// ENEMY_ATTACK ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsHurt;
		});

	// ENEMY_HURT ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return !enemyBlue0->IsHurt && !enemyBlue0->IsDeath;
		});

	// ENEMY_HURT ¨ ENEMY_DEATH
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_DEATH], [](Character* owner) {
		auto enemyBlue0 = static_cast<Enemy_Blue_0*>(owner);
		return enemyBlue0->IsDeath;
		});
}


/*------------------------------------------------------------------------------
   Enemy_Blue_0_IdleState
------------------------------------------------------------------------------*/
void Enemy_Blue_0_IdleState::OnEnter(Enemy_Blue_0* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), true);
}

void Enemy_Blue_0_IdleState::Update(Enemy_Blue_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);
	// general jobs
	owner->CheckIsFlip();		// Œü‚¢‚Ä‚¢‚é•ûŒü‚Ì”»’f
	owner->CheckIsAttack();		// UŒ‚‚·‚é‚©‚Ç‚¤‚©‚ð”»’f
	owner->DoAttackCountDown();	// UŒ‚‚ðƒJƒEƒ“ƒgƒ_ƒEƒ“
	owner->GetVisualPre()->SetFlip(!owner->GetIsFlip());
}

/*------------------------------------------------------------------------------
   Enemy_Blue_0_ChaseState
------------------------------------------------------------------------------*/
void Enemy_Blue_0_ChaseState::OnEnter(Enemy_Blue_0* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), true);
}

void Enemy_Blue_0_ChaseState::Update(Enemy_Blue_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);
	// general jobs
	owner->CheckIsFlip();		// Œü‚¢‚Ä‚¢‚é•ûŒü‚Ì”»’f
	owner->CheckIsAttack();		// UŒ‚‚·‚é‚©‚Ç‚¤‚©‚ð”»’f
	owner->DoChase();			// ƒ^[ƒQƒbƒg‚ð’Ç‚¢‚©‚¯‚é
	owner->DoAttackCountDown();	// UŒ‚‚ðƒJƒEƒ“ƒgƒ_ƒEƒ“
	owner->GetVisualPre()->SetFlip(!owner->GetIsFlip());
}

/*------------------------------------------------------------------------------
   Enemy_Blue_0_AttackState
------------------------------------------------------------------------------*/
void Enemy_Blue_0_AttackState::OnEnter(Enemy_Blue_0* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
	FrameNoOld = 0;
	// make sure no repeated hits
	HaveHit = false;
}

void Enemy_Blue_0_AttackState::OnExit(Enemy_Blue_0* owner)
{
	owner->ResetAttackCountDown();			// UŒ‚I‚í‚Á‚½‚çUŒ‚‚ÌƒJƒEƒ“ƒ^[‚ðƒŠƒZƒbƒg‚·‚é
	owner->SetAttackCollisionOnOff(false);	// ‚à‚µUŒ‚‚Ì“r’†‚ÅUŒ‚‚³‚ê‚½‚çA‚±‚Á‚¿‚ÉUŒ‚ƒtƒ‰ƒO‚ÆƒRƒŠƒWƒ‡ƒ“‚ð•Â‚¶‚é
	owner->SetIsAttack(false);
}

void Enemy_Blue_0_AttackState::Update(Enemy_Blue_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	int currentFrame = owner->GetVisualPre()->GetCurrentFrame();
	// UŒ‚ƒRƒŠƒWƒ‡ƒ“‚ðŠJ‚¯‚éƒtƒŒ[ƒ€
	if (currentFrame == 12 && FrameNoOld != currentFrame)
	{
		owner->SetAttackCollisionOnOff(true);
		SoundCodex::Get().PlaySE(SndPath::SE_Enemy_Attack_Blue_0);
	}
	// UŒ‚ƒRƒŠƒWƒ‡ƒ“‚ð•Â‚¶‚éƒtƒŒ[ƒ€
	if (currentFrame == 18 && FrameNoOld != currentFrame) owner->SetAttackCollisionOnOff(false);

	// jump attack
	if (currentFrame == 10 && FrameNoOld != currentFrame) owner->CalculateMoveVelocity(0.0f, 0.5f, 0.0f);

	// UŒ‚“–‚½‚é”»’è
	if (owner->GetAttackCollisionOnOff() && !HaveHit) HaveHit = owner->AttackCollide(-1.0f, { 0.5f ,0.1f,0.5f });

	// I‚í‚Á‚½‚çUŒ‚ƒtƒ‰ƒO‚ð•Â‚¶‚é
	if (owner->GetVisualPre()->ClipFinished()) owner->SetIsAttack(false);

	FrameNoOld = currentFrame;
}


/*------------------------------------------------------------------------------
   Enemy_Blue_0_HurtState
------------------------------------------------------------------------------*/
void Enemy_Blue_0_HurtState::OnEnter(Enemy_Blue_0* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
	// make sure not be hit repeatly
	BeAttackedTypeOld = owner->GetBeAttackedType();
	// offset
	float a = 0.3f - owner->GetMoveVelocity().y;
	float b = !owner->GetIsFlip() ? -1 : 1;
	owner->CalculateMoveVelocity(a * b, a, 0.0f);
}

void Enemy_Blue_0_HurtState::OnExit(Enemy_Blue_0* owner)
{
	owner->SetIsHurt(false);
	owner->SetBeAttackedType(Attack_Type_None);
}

void Enemy_Blue_0_HurtState::Update(Enemy_Blue_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	if (owner->GetBeAttackedType() != BeAttackedTypeOld) BeAttackedTypeOld = owner->GetBeAttackedType();

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
   Enemy_Blue_0_DeathState
------------------------------------------------------------------------------*/
void Enemy_Blue_0_DeathState::OnEnter(Enemy_Blue_0* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
	// offset
	float a = 0.3f - owner->GetMoveVelocity().y;
	float b = !owner->GetIsFlip() ? -1 : 1;
	owner->CalculateMoveVelocity(a * b, a, 0.0f);
	// set collision
	owner->SetCollisionSize({ 3.0f, 3.3f, 2.0f });

	SoundCodex::Get().PlaySE(SndPath::SE_Enemy_Dead_Blue_0);
}

void Enemy_Blue_0_DeathState::OnExit(Enemy_Blue_0* owner)
{
	owner->SetCollisionSize({ 2.5f, 1.8f, 1.0f });
}

void Enemy_Blue_0_DeathState::Update(Enemy_Blue_0* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	// I‚í‚Á‚½‚çŽg—pI‚í‚é‚ðƒZƒbƒg‚·‚é
	if (owner->GetVisualPre()->ClipFinished()) owner->Deactivate();
}