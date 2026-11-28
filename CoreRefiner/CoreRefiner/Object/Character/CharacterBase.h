#pragma once
#include "ObjectBase.h"
#include "Framework/Helpers/Mathematics/Collision3D.h"
#include "Stats.h"
#include "FSM.h"

#include <algorithm>

using namespace Collider3D;

// 角色间碰撞要扫的 tag。新增敌人类型时在此加一项。
inline constexpr Object_Type_Tag kCharacterTags[] = {
	character_Player,
	character_Enemy_T,
};

// 角色受击时的身体显隐。只计时和切换隐藏，不管无敌、扣血和死亡。
struct CharacterHitFlash
{
	// 开始一轮。计时归零，先显示。再次调用就重新走。
	void Begin() noexcept
	{
		on_ = true;
		elapsed_ = 0.0f;
		hide_ = false;
	}

	// dt 是这一帧的秒数。没在闪就返回。到点停掉并显示，否则按周期切换显隐。
	void Tick(float dt) noexcept
	{
		if (!on_)
		{
			return;
		}
		elapsed_ += dt;
		if (elapsed_ >= kSec_)
		{
			Clear();
			return;
		}
		hide_ = (static_cast<int>(elapsed_ / kPeriod_) % 2) != 0;
	}

	// 停掉这一轮并显示出来。
	void Clear() noexcept
	{
		on_ = false;
		elapsed_ = 0.0f;
		hide_ = false;
	}

	// 这一帧该不该跳过身体绘制。
	bool Hidden() const noexcept { return hide_; }
	// 这一轮还没走完。玩家用它判断无敌该不该结束。
	bool IsRunning() const noexcept { return on_; }

	// 正在走这一轮。
	bool on_{ false };
	// 这一轮已经走了多少秒。
	float elapsed_{ 0.0f };
	// 为真时这一帧不画身体。
	bool hide_{ false };
	// 一轮的总秒数。
	static constexpr float kSec_{ 0.15f };
	// 显隐切换的间隔秒数。
	static constexpr float kPeriod_{ 0.07f };
};

class CharacterBase : public ObjectBase
{
public:
	CharacterBase(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	CharacterBase(Object_Type_Tag tag, XMFLOAT3 position, XMFLOAT3 size, XMFLOAT3 collisionSize, bool onCollision)
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
		const float before = HpCurrent;
		HpCurrent += offset;
		HpCurrent = std::clamp(HpCurrent, 0.0f, GetHpMax());
		if (before > 0.0f && HpCurrent <= 0.0f)
		{
			OnHpDepleted_();
		}
	}
	float GetHpCurrent(void) const { return HpCurrent; }
	[[nodiscard]] float GetHpMax() const noexcept { return stats_.hpMax.Final(); }
	void CalculateMoveVelocity(float X, float Y, float Z)	{ MoveVelocity = (V(MoveVelocity) + Vec3{ X, Y, Z }).ToFloat3(); }
	void CalculateMoveVelocity(XMFLOAT3 offset)				{ MoveVelocity = (V(MoveVelocity) + V(offset)).ToFloat3(); }
	// up 是这一次向上的速度。不改水平速度。正在上升的速度已经达到 up 就不再加。
	void AddHop(float up) noexcept
	{
		if (up <= 0.0f)
		{
			return;
		}
		const float rising = MoveVelocity.y > 0.0f ? MoveVelocity.y : 0.0f;
		const float addY = up - rising;
		if (addY > 0.0f)
		{
			MoveVelocity.y += addY;
		}
	}
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
	// 受击频闪。玩家和敌人共用这一份，各自决定何时开始和何时推进。
	CharacterHitFlash hitFlash_{};

	[[nodiscard]] CharacterStats& Stats() noexcept { return stats_; }
	[[nodiscard]] const CharacterStats& Stats() const noexcept { return stats_; }
	// 波次开始：夹紧当前 HP。玩家会再叠加波间回血。
	void RecalcStats()
	{
		HpCurrent = std::clamp(HpCurrent, 0.0f, GetHpMax());
	}
protected:
	virtual void SetupTransitions(void) = 0;
	// Box 对场地 AABB + 对弹。非 Box 宿主必须整段 override（不要调基类）。
	virtual void MapItemCollide(void);
	// HP 刚从正值落到 0（击杀当帧）。玩家默认空；敌人在此关碰撞。
	virtual void OnHpDepleted_() {}
	void SetHpMax(float hp)
	{
		stats_.hpMax.base = hp;
		RecalcStats();
	}
	void ResetHpCurrent(void) { HpCurrent = GetHpMax(); }
	CharacterStats stats_{};
	float HpCurrent{ 0.0f };
	XMFLOAT3 PositionOld{ GetPosition() };
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };
	bool OnFloor{ false };
	bool IsAttack{ false };
	bool IsHurt{ false };
	bool IsDeath{ false };
};
