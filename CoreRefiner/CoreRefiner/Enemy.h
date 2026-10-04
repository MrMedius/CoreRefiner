#pragma once
#include "Character.h"
#include "Player.h"
#include "ColliderComponentBase.h"
#ifdef _DEBUG
#include "CubeWireframe.h"
#endif

#include "SoundCodex.h"
#include "GameStatsCodex.h"
#include "XMath.h"

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
		if (auto* col = GetComponent<ColliderComponentBase>())
		{
			col->SetEnabled(true);
		}
	}
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	// 击杀当帧关掉自身碰撞，避免尸体体积再吃下一发（含 Revive 重放弹）。
	void OnHpDepleted_() override
	{
		if (auto* col = GetComponent<ColliderComponentBase>())
		{
			col->SetEnabled(false);
		}
	}
	void DoChase(void)	// ターゲットを追いかける
	{
		if (!IsInAttackArea)
		{
			const auto selfPos = GetPosition();
			const Vec3 d = V(AttackTarget->GetPosition()) - V(selfPos);

			// ターゲットの方向を向く
			float angle = atan2f(d.x, d.z);
			SetRotation({ 0.0f, XMConvertToDegrees(angle), 0.0f });

			// ターゲットに向かって移動
			CalculateMoveVelocity(sinf(angle) * GetMoveAccel(), 0.0f, cosf(angle) * GetMoveAccel());
		}
	}
	bool CheckIsAttack(void)	// 攻撃するかどうかを判断
	{
		if (IsInAttackArea) SetIsAttack(true);
		return IsAttack;
	}
	// Melee hit test against AttackTarget via shape-agnostic GetVolume (Player may be Capsule).
	bool AttackCollide(float damage, XMFLOAT3 repel) override
	{
		if (AttackTarget == nullptr)
		{
			return false;
		}
		auto* selfCol = GetComponent<ColliderComponentBase>();
		auto* targetCol = AttackTarget->GetComponent<ColliderComponentBase>();
		if (selfCol == nullptr || targetCol == nullptr || !targetCol->IsEnabled())
		{
			return false;
		}
		if (!AttackTarget->GetIsHurt() && !AttackTarget->GetIsDeath())
		{
			// Do not use GetBoxCollider — target may be Capsule (Player).
			bool isHit = CollisionSystem::IsOverlap(selfCol->GetVolume(), targetCol->GetVolume());

			if (isHit)
			{
				AttackTarget->SetIsHurt(true);
				AttackTarget->CalculateHpCurrent(damage);
				GameStatsCodex::AddInputDamage(-damage);

				const auto selfPos = GetPosition();
				const Vec3 d = V(AttackTarget->GetPosition()) - V(selfPos);
				float angle = atan2f(d.x, d.z);
				AttackTarget->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z);
				return true;
			}
		}
		return false;
	}

	// 贴身碰撞伤害 + 击退；Hurt 期间由玩家无敌帧挡住连击。
	void TryContactHit()
	{
		if (GetIsDeath() || GetIsHurt())
		{
			return;
		}
		AttackCollide(kContactHpDelta_, kContactRepel_);
	}
	// 玩家碰撞推开：只改 XZ 并立刻 Sync；消掉朝推开反方向（往玩家钻）的水平速度。
	// dx 世界 X 增量。
	// dz 世界 Z 增量。
	void ShoveXZ(float dx, float dz)
	{
		auto p = GetPosition();
		p.x += dx;
		p.z += dz;
		SetPosition(p);
		if (auto* col = GetComponent<ColliderComponentBase>())
		{
			col->SyncFromOwner();
		}

		XMFLOAT3 away{ dx, 0.0f, dz };
		if (!NormalizeXZ(away))
		{
			return;
		}
		const float vn = MoveVelocity.x * away.x + MoveVelocity.z * away.z;
		if (vn < 0.0f)
		{
			MoveVelocity.x -= vn * away.x;
			MoveVelocity.z -= vn * away.z;
		}
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
	// 先 Character 场地/对弹，再怪互挤。对玩家不写位置、不消速度。
	void MapItemCollide() override;
	void SetEnemyType(ENEMY_TYPE_ID type) { Type = type; }
	void SetSearchArea(XMFLOAT3 area)	  { searchCollider.half = (V(area) * 0.5f).ToFloat3(); }
	XMFLOAT3 GetSearchArea(void) const	  { return searchCollider.half; }
	// Search-volume vs target GetVolume (searchCollider stays Box; target may be Capsule).
	bool CheckIsInArea(void)
	{
		auto* targetCol = AttackTarget->GetComponent<ColliderComponentBase>();
		if (targetCol == nullptr)
		{
			return IsInArea = false;
		}
		return IsInArea = CollisionSystem::IsOverlap(searchCollider, targetCol->GetVolume());
	}
private:
	void SetupTransitions(void) override = 0;
protected:
	// 碰撞扣血（CalculateHpCurrent 的偏移，负数为受伤）。
	static constexpr float kContactHpDelta_ = -5.0f;
	// 水平击退 + 小幅上抬。
	static constexpr XMFLOAT3 kContactRepel_{ 0.40f, 0.01f, 0.40f };
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
