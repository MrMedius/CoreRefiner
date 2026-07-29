#pragma once
#include "ObjectBase.h"
#include "Collision.h"
#include "FSM.h"

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
	void CalculateHpCurrent(float offset)	{ HpCurrent += offset; HpCurrent = std::clamp(HpCurrent, 0.0f, HpMax); } // 今の体力の計算
	float GetHpCurrent(void) const			{ return HpCurrent; }													 // 今の体力をゲット
	void CalculateMoveVelocity(float X, float Y, float Z)	{ MoveVelocity.x += X; MoveVelocity.y += Y; MoveVelocity.z += Z; }	// 移動値をセット
	void CalculateMoveVelocity(XMFLOAT3 offset)				{ CalculateMoveVelocity(offset.x, offset.y, offset.z); }			// 移動値をセット
	XMFLOAT3 GetMoveVelocity(void) const					{ return MoveVelocity; }											// 移動値をゲット
	void SetMoveAccel(float accel)							{ MoveAccel = accel; }	// 移動加速値をセット
	float GetMoveAccel(void) const							{ return MoveAccel; }	// 移動加速値をゲット
	void ResetAttackCountDown(void)							{ AttackCountDown = AttackInterval; }			// 攻撃のコリジョンをリセット
	void DoAttackCountDown(void)							{ if (AttackCountDown > 0) AttackCountDown--; }	// 攻撃をカウントダウン
	void SetIsAttack(bool state)	{ IsAttack = state; }		// 攻撃状態をセット
	bool GetIsAttack(void) const	{ return IsAttack; }		// 攻撃状態をゲット
	void SetIsHurt(bool state)		{ IsHurt = state; }			// 攻撃された状態をセット
	bool GetIsHurt(void) const		{ return IsHurt; }			// 攻撃された状態をゲット
	void SetIsDeath(bool state)		{ IsDeath = state; }		// 死亡状態をセット
	bool GetIsDeath(void) const		{ return IsDeath; }			// 死亡状態をゲット
	virtual bool AttackCollide(float damage, XMFLOAT3 repel) = 0;						// 攻撃コリジョンの仮想関数
protected:
	virtual void SetupTransitions(void) = 0;	// 状態遷移条件
	/**
	 * @brief Default Box AABB resolve (Enemy path). Non-Box hosts must override.
	 */
	virtual void MapItemCollide(void);
	void SetHpMax(float hp)					{ HpMax = hp; }							// 最大体力をセット
	void ResetHpCurrent(void)				{ HpCurrent = HpMax; }					// 今の体力をリセット
	void SetAttackInterval(float second)	{ AttackInterval = second * 60.0f; }	// 攻撃間隔をセット
	float GetAttackInterval(void) const		{ return AttackInterval; }				// 攻撃間隔をゲット
	void SetAttackCountDown(float count)	{ AttackCountDown = count; }			// 攻撃間隔カウントダウンをセット
	float GetAttackCountDown(void) const	{ return AttackCountDown; }				// 攻撃間隔カウントダウンをゲット
protected:
	static constexpr float GRAVITY = 1.0f;				// 重力
	static constexpr float 	FORCE_RATE = 0.1f;			// 摩擦力
	float HpMax{ 0.0f };								// 最大体力
	float HpCurrent{ 0.0f };							// 今の体力
	XMFLOAT3 PositionOld{ GetPosition() };				// 1フレーム前の座標
	float MoveAccel{ 0.0f };							// 移動加速値
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };			// 移動値
	float AttackInterval{ 0 };							// 攻撃間隔
	float AttackCountDown{ 0 };							// 攻撃間隔カウントダウン
	bool OnFloor{ false };								// 地面に乗っているかどうかの判断
	bool IsAttack{ false };								// 攻撃状態の判断
	bool IsHurt{ false };								// 攻撃された状態の判断
	bool IsDeath{ false };								// 死亡状態の判断
};