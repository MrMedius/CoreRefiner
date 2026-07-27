#pragma once
#include "Character.h"
#include "Player.h"
#include "ColliderComponent.h"
#ifdef _DEBUG
#include "CubeWireframe.h"
#endif

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
			const auto selfPos = GetPosition();
			float dx = AttackTarget->GetPosition().x - selfPos.x;
			float dz = AttackTarget->GetPosition().z - selfPos.z;

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
		auto* selfCol = GetComponent<ColliderComponent>();
		auto* targetCol = AttackTarget->GetComponent<ColliderComponent>();
		if (selfCol == nullptr || targetCol == nullptr || !targetCol->IsEnabled())
		{
			return false;
		}
		if (!AttackTarget->GetIsHurt() && !AttackTarget->GetIsDeath())
		{
			bool isHit = CollisionSystem::IsOverlap(selfCol->GetBoxCollider(), targetCol->GetBoxCollider());

			if (isHit)
			{
				AttackTarget->SetIsHurt(true);
				AttackTarget->CalculateHpCurrent(damage);
				GameStatsCodex::AddInputDamage(-damage);

				const auto selfPos = GetPosition();
				float dx = AttackTarget->GetPosition().x - selfPos.x;
				float dz = AttackTarget->GetPosition().z - selfPos.z;
				float angle = atan2f(dx, dz);
				AttackTarget->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z);
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
protected:
	void SetEnemyType(ENEMY_TYPE_ID type) { Type = type; }
	void SetSearchArea(XMFLOAT3 area)	  { searchCollider.half = { area.x / 2,area.y / 2 ,area.z / 2 }; }
	XMFLOAT3 GetSearchArea(void) const	  { return searchCollider.half; }
	bool CheckIsInArea(void)
	{
		auto* targetCol = AttackTarget->GetComponent<ColliderComponent>();
		if (targetCol == nullptr)
		{
			return IsInArea = false;
		}
		return IsInArea = CollisionSystem::IsOverlap(searchCollider, targetCol->GetBoxCollider());
	}
private:
	void SetupTransitions(void) override = 0;
protected:
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
