#pragma once
#include "Character.h"
#include "Enemy_T_Shape.h"
#include "Player.h"

#include "SoundCodex.h"
#include "GameStatsCodex.h"

// 敵状態のID
enum ENEMY_STATE_ID {
	ENEMY_CHASE,
	ENEMY_ATTACK,
	ENEMY_HURT,
	ENEMY_DEATH,
};
// 敵状態の名前
static const std::string ENEMY_STATE[] = {
	"ENEMY_CHASE",
	"ENEMY_ATTACK",
	"ENEMY_HURT",
	"ENEMY_DEATH",
};
// 敵の種類
enum ENEMY_TYPE_ID {
	ENEMY_TYPE_NONE,

	ENEMY_TYPE_T,
};


class Enemy : public Character
{
public:
	Enemy(Object_Type_Tag tag)
		:
		Character(tag)
	{
		AttackTarget = ObjectCodex::FindFirstObjectByTag<Player>(character_Player);
	}
	void SpawnAt(XMFLOAT3 pos)
	{
		SetPosition(pos);
	}
	void OnEnable(void) override
	{
		ResetHpCurrent();
		SetIsDeath(false);
	}
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	void DoChase(void)	// ターゲットを追いかける
	{
		if (!IsInAttackArea)
		{
			float dx = AttackTarget->GetPosition().x - transInfo.position.x;
			float dz = AttackTarget->GetPosition().z - transInfo.position.z;

			// ターゲットの方向を向く
			float angle = atan2f(dx, dz);
			SetRotation({ 0.0f, XMConvertToDegrees(angle), 0.0f });

			// ターゲットに向かって移動
			CalculateMoveVelocity(sinf(angle) * GetMoveAccel(), 0.0f, cosf(angle) * GetMoveAccel());
		}
	}
	bool CheckIsAttack(void)	// 攻撃するかどうかを判断
	{
		if (IsInAttackArea && AttackCountDown == 0) SetIsAttack(true);
		return IsAttack;
	}
	bool AttackCollide(float damage, XMFLOAT3 repel) override
	{
		if (!AttackTarget->GetIsHurt() && !AttackTarget->GetIsDeath() && AttackTarget->GetCollisionOnOff())
		{
			bool isHit = CollisionSystem::IsOverlap(GetBoxCollider(), AttackTarget->GetBoxCollider());

			if (isHit)
			{
				AttackTarget->SetIsHurt(true);
				AttackTarget->CalculateHpCurrent(damage);
				GameStatsCodex::AddInputDamage(-damage);

				float dx = AttackTarget->GetPosition().x - transInfo.position.x;
				float dz = AttackTarget->GetPosition().z - transInfo.position.z;
				float angle = atan2f(dx, dz);
				AttackTarget->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z); // 撃退する
				return true;
			}
		}
		return false;
	}
	void SetWasHurt(bool state) { WasHurt = state; }
	bool GetWasHurt(void) 
	{ 
		if (WasHurt)
		{
			WasHurt = false;
			return true;
		}
		else
			return false;
	}
	virtual int GetKillScore() const noexcept = 0;
	ENEMY_TYPE_ID GetEnemyType(void) const { return Type; }
	ObjectBase* GetAttackTarget(void) { return AttackTarget; }
	Enemy_T_Shape* GetVisualPre(void) { return visualPre.get(); }
protected:
	void SetEnemyType(ENEMY_TYPE_ID type) { Type = type; }
	void SetSearchArea(XMFLOAT3 area)	  { searchCollider.half = { area.x / 2,area.y / 2 ,area.z / 2 }; }
	XMFLOAT3 GetSearchArea(void) const	  { return searchCollider.half; }
	bool CheckIsInArea(void)
	{
		return IsInArea = CollisionSystem::IsOverlap(searchCollider, AttackTarget->GetBoxCollider());
	}
private:
	void SetupTransitions(void) override = 0;
protected:
	std::unique_ptr<Enemy_T_Shape> visualPre;
	ENEMY_TYPE_ID Type{ ENEMY_TYPE_NONE };	// 敵の種類
	Player* AttackTarget;					// ターゲット
	BoxCollider searchCollider;				// 検査のコリジョン
	bool IsInArea{ false };					// ターゲットが検査範囲にいる
	bool IsInAttackArea{ false };			// ターゲットが攻撃範囲にいる
	bool WasHurt{ false };					// 攻撃されたかどうかのフラグ
#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> searchColliderWire;
#endif
};