#pragma once
#include "ObjectBase.h"
#include "Collision3D.h"
#include "Stats.h"
#include "FSM.h"

#include <algorithm>

using namespace Collider3D;

class Character : public ObjectBase
{
public:
	Character(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	Character(Object_Type_Tag tag, XMFLOAT3 position, XMFLOAT3 size, XMFLOAT3 collisionSize, bool onCollision)
		:
		ObjectBase(tag)
	{
		(void)collisionSize; // set via shape collider SetCollisionSize / SetCapsule after AddComponent
		(void)onCollision;   // set via ColliderComponentBase::SetEnabled after AddComponent
		SetPosition(position);
		SetSize(size);
	}
	virtual void OnEnable(void) override = 0;
	virtual void Update(float dt) override = 0;
	virtual void Submit(void) override = 0;
	void CalculateHpCurrent(float offset)
	{
		HpCurrent += offset;
		HpCurrent = std::clamp(HpCurrent, 0.0f, GetHpMax());
	}
	float GetHpCurrent(void) const { return HpCurrent; }
	[[nodiscard]] float GetHpMax() const noexcept { return stats_.hpMax.Final(); }
	void CalculateMoveVelocity(float X, float Y, float Z)	{ MoveVelocity = (V(MoveVelocity) + Vec3{ X, Y, Z }).ToFloat3(); }
	void CalculateMoveVelocity(XMFLOAT3 offset)				{ MoveVelocity = (V(MoveVelocity) + V(offset)).ToFloat3(); }
	XMFLOAT3 GetMoveVelocity(void) const					{ return MoveVelocity; }
	void SetMoveAccel(float accel) { stats_.moveAccel.base = accel; }
	float GetMoveAccel(void) const { return stats_.moveAccel.Final(); }
	[[nodiscard]] float GetGravity() const noexcept { return stats_.gravity.Final(); }
	[[nodiscard]] float GetMoveFriction() const noexcept { return stats_.moveFriction.Final(); }
	void SetIsAttack(bool state)	{ IsAttack = state; }
	bool GetIsAttack(void) const	{ return IsAttack; }
	void SetIsHurt(bool state)		{ IsHurt = state; }
	bool GetIsHurt(void) const		{ return IsHurt; }
	void SetIsDeath(bool state)		{ IsDeath = state; }
	bool GetIsDeath(void) const		{ return IsDeath; }
	virtual bool AttackCollide(float damage, XMFLOAT3 repel) = 0;

	[[nodiscard]] CharacterStats& Stats() noexcept { return stats_; }
	[[nodiscard]] const CharacterStats& Stats() const noexcept { return stats_; }
	/** @brief 波次开始：夹紧当前 HP。玩家会再叠加波间回血。 */
	void RecalcStats()
	{
		HpCurrent = std::clamp(HpCurrent, 0.0f, GetHpMax());
	}
protected:
	virtual void SetupTransitions(void) = 0;
	/**
	 * @brief Default Box AABB resolve (Enemy path). Non-Box hosts must override.
	 */
	virtual void MapItemCollide(void);
	void SetHpMax(float hp)
	{
		stats_.hpMax.base = hp;
		RecalcStats();
	}
	void ResetHpCurrent(void) { HpCurrent = GetHpMax(); }
protected:
	CharacterStats stats_{};
	float HpCurrent{ 0.0f };
	XMFLOAT3 PositionOld{ GetPosition() };
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };
	bool OnFloor{ false };
	bool IsAttack{ false };
	bool IsHurt{ false };
	bool IsDeath{ false };
};
