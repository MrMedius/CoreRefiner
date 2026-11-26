#pragma once
#include "AttackBase.h"
#include "Ring_Shape.h"
#include "Channels.h"
#include "EnemyBase.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

#include <cmath>
#include <memory>
#include <vector>

// 阳的攻击环。外沿扫过时打中敌人并向上轻跳。环带内沿空、外沿白，约 0.3 秒扩到目标半径。
class RingYang : public AttackBase
{
public:
	RingYang(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		AttackBase(attack_Ring_Yang)
	{
		// 这一步没有伤害。base 是基础值，ResetMods 清掉加算、倍率和固定补正。
		Stats().damage.base = 0.0f;
		Stats().damage.ResetMods();
		ring_ = std::make_unique<Ring_Shape>(gfx);
		ring_->LinkTechniques(rg);
		// 内沿空，外沿白。播放淡出写在 opacity 上，不把内沿当成有颜色。
		ring_->SetInnerColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		ring_->SetOuterColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		ring_->SetInnerWave(true);
		ring_->SetOuterWave(true);
		ring_->SetAmplitude(kAmplitude_);
	}

	// pos 是圆心。dir 仍记下来并交给下一只调用。受击不再用它改水平速度。
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		origin_ = pos;
		hitDir_ = dir;
		age_ = 0.0f;
		prevOuter_ = 0.0f;
		outer_ = 0.0f;
		targetRadius_ = kDefaultRadius_;
		hitEnemies_.clear();
		Stats().damage.base = 0.0f;
		Stats().damage.ResetMods();
		SetPosition(pos);
		ResetMoveVelocity();
		SetMoveAccel({ 0.0f, 0.0f, 0.0f });
		if (ring_ != nullptr)
		{
			ring_->SetAmplitude(kAmplitude_);
		}
	}

	// 这次要扩到的世界半径。须在 SpawnAt 之后写，出生会把它收回默认 3。
	void SetTargetRadius(float radius) noexcept { targetRadius_ = radius; }

	void OnEnable(void) override
	{
		age_ = 0.0f;
		prevOuter_ = 0.0f;
		outer_ = 0.0f;
		hitEnemies_.clear();
	}

	void Update(float dt) override
	{
		prevOuter_ = outer_;
		age_ += dt;
		outer_ = OuterAt_(age_);
		SweepWavefront_();
		if (age_ >= kExpandSeconds_ + kFadeSeconds_)
		{
			RequestDisable();
		}
		ObjectBase::Update(dt);
	}

	void Submit(void) override
	{
		if (ApplyRing_() && ring_ != nullptr)
		{
			ring_->Submit(Chan::main);
		}
		ObjectBase::Submit();
	}

protected:
	// 扣 Stats().damage.Final()，让敌人进入受伤并向上轻跳。同一个人只打一次。伤害为 0 时什么都不做。
	void ApplyOwnHit(CharacterBase* other) override
	{
		auto* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr || enemy->GetIsDeath())
		{
			return;
		}
		// damage 是这次要扣的血。为 0 时只播放，不进入受伤，不跳。
		const float damage = Stats().damage.Final();
		if (damage == 0.0f)
		{
			return;
		}
		for (EnemyBase* hit : hitEnemies_)
		{
			if (hit == enemy)
			{
				return;
			}
		}
		hitEnemies_.push_back(enemy);
		enemy->SetIsHurt(true);
		enemy->CalculateHpCurrent(-damage);
		// 不加水平速度。正在上升时不超过 kKnockbackUp_。
		enemy->AddHop(kKnockbackUp_);
	}

private:
	// 外沿这一帧往外走时，扫到刚被外沿越过的活人。含刚离开圆心的那个人。淡出时外沿不动，不再打新人。
	void SweepWavefront_()
	{
		// 伤害为 0 只播放。外沿没往外走不再产生新的命中。
		if (Stats().damage.Final() == 0.0f || !(outer_ > prevOuter_))
		{
			return;
		}
		for (Object_Type_Tag tag : kCharacterTags)
		{
			for (EnemyBase* enemy : ObjectCodex::FindActiveObjectsByTag<EnemyBase>(tag))
			{
				if (enemy == nullptr || enemy->GetIsDeath())
				{
					continue;
				}
				const XMFLOAT3 pos = enemy->GetWorldPosition();
				const float dx = pos.x - origin_.x;
				const float dz = pos.z - origin_.z;
				const float distSq = dx * dx + dz * dz;
				// atCenter 是站在圆心上的那个人。距离为 0，普通比较会漏掉他。
				const bool atCenter = distSq <= 1.0e-8f;
				const float dist = atCenter ? 0.0f : std::sqrt(distSq);
				const bool crossed = prevOuter_ < dist && dist <= outer_;
				const bool centerLeave = atCenter && prevOuter_ <= 0.0f && outer_ > prevOuter_;
				if (!crossed && !centerLeave)
				{
					continue;
				}
				ApplyOwnHit(enemy);
			}
		}
	}

	// age 对应的外沿世界半径。淡出阶段停在目标半径。
	float OuterAt_(float age) const
	{
		if (age <= 0.0f)
		{
			return 0.0f;
		}
		if (age < kExpandSeconds_)
		{
			return targetRadius_ * (age / kExpandSeconds_);
		}
		return targetRadius_;
	}

	// 按年龄写下环的位置、缩放和播放透明度。还在播放返回 true。
	bool ApplyRing_()
	{
		if (ring_ == nullptr || age_ <= 0.0f)
		{
			return false;
		}
		// opacity 是整只环的播放透明度。扩大时保持 1，淡出时从 1 收到 0。
		float opacity = 0.0f;
		if (age_ < kExpandSeconds_)
		{
			opacity = 1.0f;
		}
		else if (age_ < kExpandSeconds_ + kFadeSeconds_)
		{
			const float fade = (age_ - kExpandSeconds_) / kFadeSeconds_;
			opacity = 1.0f - fade;
		}
		else
		{
			return false;
		}
		ring_->SetPosition(origin_);
		ring_->SetScale({ outer_, outer_, outer_ });
		ring_->SetOpacity(opacity);
		return true;
	}

	// 这一只白环。内沿空、外沿白写在构造里。
	std::unique_ptr<Ring_Shape> ring_;
	// 这一次播放的圆心。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// SpawnAt 传入的水平方向。现在只保存并传递，受击不读取。
	XMFLOAT3 hitDir_{ 0.0f, 0.0f, 0.0f };
	// 从 SpawnAt 起算的秒数。
	float age_{ 0.0f };
	// 上一帧外沿的世界半径。
	float prevOuter_{ 0.0f };
	// 这一帧外沿的世界半径。
	float outer_{ 0.0f };
	// 这次要扩到的世界半径。
	float targetRadius_{ 3.0f };
	// 这只环已经打过的敌人。再次出生时清空。
	std::vector<EnemyBase*> hitEnemies_;

	// 从 0 扩到目标半径用的秒数。
	static constexpr float kExpandSeconds_{ 0.3f };
	// 扩完之后淡出用的秒数。淡出时外沿不动。
	static constexpr float kFadeSeconds_{ 0.25f };
	// 出生时写回的目标半径。模型外径是 1。
	static constexpr float kDefaultRadius_{ 3.0f };
	// 波浪幅度。
	static constexpr float kAmplitude_{ 0.35f };
	// 受击时向上的速度。不加水平速度。正在上升时不超过这一次。
	static constexpr float kKnockbackUp_{ 0.1f };
};
