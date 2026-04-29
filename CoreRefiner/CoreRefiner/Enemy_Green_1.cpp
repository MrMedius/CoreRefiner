#include "Enemy_Green_1.h"
#include "Channels.h"

void Enemy_Green_1::Update(float dt)
{
	// •ÏX‘O‚ÌÀ•W‚ðŠi”[
	auto Position = transInfo.position;
	PositionOld = Position;
	searchCollider.center = Position;
	CheckIsInArea();

	// UŒ‚”ÍˆÍ‚ÌXV
	if (IsInArea)
	{
		if (!IsFlip) attackCollider.center = { Position.x + boxCollider.half.x + attackCollider.half.x, Position.y, Position.z };
		else attackCollider.center = { Position.x - boxCollider.half.x - attackCollider.half.x, Position.y, Position.z };
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

void Enemy_Green_1::Submit(void)
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

void Enemy_Green_1::SetupTransitions(void)
{
	// ó‘Ô‚Ì‘JˆÚðŒ‚ð‘‰Á‚·‚é
	// ENEMY_IDLE ¨ ENEMY_CHASE
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_CHASE], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsInArea && !enemyGreen0->IsInAttackArea; // ”»’f‚ÌðŒ
		});

	// ENEMY_IDLE ¨ ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsAttack;
		});

	// ENEMY_IDLE ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_IDLE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsHurt;
		});

	// ENEMY_CHASE ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return (!enemyGreen0->IsInArea) || (enemyGreen0->IsInAttackArea && enemyGreen0->AttackCountDown > 0);
		});

	// ENEMY_CHASE ¨ ENEMY_ATTACK
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_ATTACK], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsAttack;
		});

	// ENEMY_CHASE ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_CHASE], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsHurt;
		});

	// ENEMY_ATTACK ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return !enemyGreen0->IsAttack;
		});

	// ENEMY_ATTACK ¨ ENEMY_HURT
	FSM->AddTransition(ENEMY_STATE[ENEMY_ATTACK], ENEMY_STATE[ENEMY_HURT], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsHurt;
		});

	// ENEMY_HURT ¨ ENEMY_IDLE
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_IDLE], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return !enemyGreen0->IsHurt && !enemyGreen0->IsDeath;
		});

	// ENEMY_HURT ¨ ENEMY_DEATH
	FSM->AddTransition(ENEMY_STATE[ENEMY_HURT], ENEMY_STATE[ENEMY_DEATH], [](Character* owner) {
		auto enemyGreen0 = static_cast<Enemy_Green_1*>(owner);
		return enemyGreen0->IsDeath;
		});
}


/*------------------------------------------------------------------------------
   Enemy_Green_1_IdleState
------------------------------------------------------------------------------*/
void Enemy_Green_1_IdleState::OnEnter(Enemy_Green_1* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), true);
}

void Enemy_Green_1_IdleState::Update(Enemy_Green_1* owner, float dt)
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
   Enemy_Green_1_ChaseState
------------------------------------------------------------------------------*/
void Enemy_Green_1_ChaseState::OnEnter(Enemy_Green_1* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), true);
}

void Enemy_Green_1_ChaseState::Update(Enemy_Green_1* owner, float dt)
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
   Enemy_Green_1_AttackState
------------------------------------------------------------------------------*/
void Enemy_Green_1_AttackState::OnEnter(Enemy_Green_1* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
	FrameNoOld = 0;
	// make sure no repeated hits
	HaveHit = false;
}

void Enemy_Green_1_AttackState::OnExit(Enemy_Green_1* owner)
{
	owner->ResetAttackCountDown();			// UŒ‚I‚í‚Á‚½‚çUŒ‚‚ÌƒJƒEƒ“ƒ^[‚ðƒŠƒZƒbƒg‚·‚é
	owner->SetAttackCollisionOnOff(false);	// ‚à‚µUŒ‚‚Ì“r’†‚ÅUŒ‚‚³‚ê‚½‚çA‚±‚Á‚¿‚ÉUŒ‚ƒtƒ‰ƒO‚ÆƒRƒŠƒWƒ‡ƒ“‚ð•Â‚¶‚é
	owner->SetIsAttack(false);
}

void Enemy_Green_1_AttackState::Update(Enemy_Green_1* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	int currentFrame = owner->GetVisualPre()->GetCurrentFrame();
	// UŒ‚ƒRƒŠƒWƒ‡ƒ“‚ðŠJ‚¯‚éƒtƒŒ[ƒ€
	if (currentFrame == 15 && FrameNoOld != currentFrame) owner->SetAttackCollisionOnOff(true);
	// UŒ‚ƒRƒŠƒWƒ‡ƒ“‚ð•Â‚¶‚éƒtƒŒ[ƒ€
	if (currentFrame == 16 && FrameNoOld != currentFrame) owner->SetAttackCollisionOnOff(false);

	// UŒ‚“–‚½‚é”»’è
	if (owner->GetAttackCollisionOnOff() && !HaveHit) HaveHit = owner->AttackCollide(-2.0f, { 1.0f ,0.4f,0.8f });

	// I‚í‚Á‚½‚çUŒ‚ƒtƒ‰ƒO‚ð•Â‚¶‚é
	if (owner->GetVisualPre()->ClipFinished()) owner->SetIsAttack(false);

	FrameNoOld = currentFrame;
}


/*------------------------------------------------------------------------------
   Enemy_Green_1_HurtState
------------------------------------------------------------------------------*/
void Enemy_Green_1_HurtState::OnEnter(Enemy_Green_1* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
	// make sure not be hit repeatly
	BeAttackedTypeOld = owner->GetBeAttackedType();
}

void Enemy_Green_1_HurtState::OnExit(Enemy_Green_1* owner)
{
	owner->SetIsHurt(false);
	owner->SetBeAttackedType(Attack_Type_None);
}

void Enemy_Green_1_HurtState::Update(Enemy_Green_1* owner, float dt)
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
   Enemy_Green_1_DeathState
------------------------------------------------------------------------------*/
void Enemy_Green_1_DeathState::OnEnter(Enemy_Green_1* owner)
{
	// set anime
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
}

void Enemy_Green_1_DeathState::Update(Enemy_Green_1* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);

	// I‚í‚Á‚½‚çŽg—pI‚í‚é‚ðƒZƒbƒg‚·‚é
	if (owner->GetVisualPre()->ClipFinished()) owner->Deactivate();
}