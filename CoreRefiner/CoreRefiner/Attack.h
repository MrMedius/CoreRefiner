#pragma once
#include "ObjectBase.h"
#include "Character.h"
#include "Stats.h"

#include <vector>

// 一次攻击的公共部分：出生、位移、寿命、数值，以及自己的命中。
// 玩法模块、配方和连发收养在 Attack_Moduled 里。
class Attack : public ObjectBase
{
public:
	Attack(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	void virtual SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) = 0;
	void OnEnable(void) override = 0;
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	// 只结算这发攻击自己的命中。带模组的弹会覆盖它，命中后再跑模块。
	virtual void OnCollide(Character* other)
	{
		ApplyOwnHit(other);
	}

	// 本弹数值袋（damage / size / speed / lifetime）。
	[[nodiscard]] AttackStats& Stats() noexcept { return stats_; }
	[[nodiscard]] const AttackStats& Stats() const noexcept { return stats_; }

	// 标记这颗弹已经出生。打开碰撞之前调用。
	void MarkLaunched() noexcept { launched_ = true; }
	// 是否已经出生。未出生时碰撞保持关闭。
	[[nodiscard]] bool IsLaunched() const noexcept { return launched_; }

	void CalculateMoveVelocity(float X, float Y, float Z) { MoveVelocity = (V(MoveVelocity) + Vec3{ X, Y, Z }).ToFloat3(); }
	void CalculateMoveVelocity(XMFLOAT3 offset) { MoveVelocity = (V(MoveVelocity) + V(offset)).ToFloat3(); }
	void ResetMoveVelocity(void) { MoveVelocity = { 0.0f,0.0f,0.0f }; }
	XMFLOAT3 GetMoveVelocity(void) const { return MoveVelocity; }
	void SetMoveAccel(XMFLOAT3 accel) { MoveAccel = accel; }
	XMFLOAT3 GetMoveAccel(void) const { return MoveAccel; }

	// 写入本地位置。带模组的弹用来摆开火点。
	void SetLocalPosition(XMFLOAT3 position) { SetPosition(position); }
	// 写入本地缩放。
	void SetLocalScale(XMFLOAT3 scale) { SetSize(scale); }

	// 开火时拍板的寿命秒数。
	void SetLifeTime(float seconds) { lifeTime = seconds; }
	[[nodiscard]] float GetLifeTime() const noexcept { return lifeTime; }
	// 出生时把已活过的时间归零。
	void ResetLifeTimer() { lastTime = 0.0f; }
	// 累加已活过的时间。寿命到了返回 true。
	[[nodiscard]] bool TickLifeTimer(float dt)
	{
		lastTime += dt;
		return lastTime >= lifeTime;
	}

protected:
	// 这发攻击自己的命中：伤害、击退、停下。不要在这里调用模块。
	virtual void ApplyOwnHit(Character* other) = 0;

	// 回收时清掉“已经出生”。带模组的弹清模块时调用。
	void ClearLaunched_() noexcept { launched_ = false; }

	// 更新还活着的子物体。
	void UpdateChildren(float dt)
	{
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i); child != nullptr && child->IsActive())
			{
				child->Update(dt);
			}
		}
	}
	// 提交还活着的子物体。
	void SubmitChildren()
	{
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i); child != nullptr && child->IsActive())
			{
				child->Submit();
			}
		}
	}

protected:
	XMFLOAT3 MoveAccel{ 0.0f,0.0f,0.0f };
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };
	float lastTime{ 0.0f };
	float lifeTime{ 2.0f };

private:
	AttackStats stats_{};
	// 是否已经 SpawnAt。回收后回到未出生。
	bool launched_{ false };
};
