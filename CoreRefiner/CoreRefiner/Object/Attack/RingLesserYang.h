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

// 少阳的攻击环。扩开时打中半径内的每个活人，含圆心。打完才加阳标记，满 10 个再放下一只。
// 受击和阳环一样，只向上轻跳。环带外沿白、内沿黑，约 0.35 秒扩到目标半径。
class RingLesserYang : public AttackBase
{
public:
	RingLesserYang(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		AttackBase(attack_Ring_LesserYang),
		gfx_(gfx),
		rg_(rg)
	{
		Stats().damage.base = 0.0f;
		Stats().damage.ResetMods();
		ring_ = std::make_unique<Ring_Shape>(gfx);
		ring_->LinkTechniques(rg);
		// 内沿不透明黑，外沿不透明白。两边都有颜色，沿环带从黑收到白。不起浪。
		ring_->SetInnerColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		ring_->SetOuterColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		ring_->SetInnerWave(false);
		ring_->SetOuterWave(false);
		ring_->SetAmplitude(0.0f);
	}

	// pos 是圆心。dir 仍记下来并交给下一只环。受击不再用它改水平速度。
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
	}

	// damage 是这只环扣的固定血量。radius 是要扩到的水平半径。markCount 是打中后给予的个数。
	// burstsLeft 是这一次命中还剩几只环。同一轮放出去的环共用它。须在 SpawnAt 之后写。
	void SetBurst(float damage, float radius, int markCount, std::shared_ptr<int> burstsLeft) noexcept
	{
		areaDamage_ = damage;
		markCount_ = markCount;
		burstsLeft_ = std::move(burstsLeft);
		targetRadius_ = radius;
		Stats().damage.base = damage;
		Stats().damage.ResetMods();
	}

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
		SweepDisk_();
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
	// 扣这只环自己的固定伤害，并按阳环向上轻跳。同一个人只打一次。打中之后才加标记。
	void ApplyOwnHit(CharacterBase* other) override
	{
		auto* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr || enemy->GetIsDeath())
		{
			return;
		}
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
		GrantAndChain_(*enemy);
	}

private:
	// 外沿还在往外走时，扫进当前半径的活人。含圆心。淡出时外沿不动，不再打新人。
	void SweepDisk_()
	{
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
				if (distSq <= outer_ * outer_)
				{
					ApplyOwnHit(enemy);
				}
			}
		}
	}

	// 加上给予个数。够 10 个就再放一只环，并扣掉共用的剩余次数。
	void GrantAndChain_(EnemyBase& enemy)
	{
		if (markCount_ <= 0)
		{
			return;
		}
		enemy.Marks().Add(AttackMark::Yang, markCount_);
		while (burstsLeft_ != nullptr && *burstsLeft_ > 0 && enemy.Marks().Trigger(AttackMark::Yang, kPerTrigger_) > 0)
		{
			SpawnNext_(enemy);
		}
	}

	// 在这个敌人脚下再放一只。出生方向用这一只环的 hitDir_。次数先扣，池里没有再还回去。
	void SpawnNext_(const EnemyBase& enemy)
	{
		if (burstsLeft_ == nullptr || *burstsLeft_ <= 0)
		{
			return;
		}
		*burstsLeft_ -= 1;
		RingLesserYang* ring = ObjectCodex::SpawnPooled<RingLesserYang>(attack_Ring_LesserYang, gfx_, rg_);
		if (ring == nullptr)
		{
			*burstsLeft_ += 1;
			return;
		}
		ring->SpawnAt(enemy.GetWorldPosition(), hitDir_);
		ring->SetBurst(areaDamage_, targetRadius_, markCount_, burstsLeft_);
		ObjectCodex::Handoff(attack_Ring_LesserYang, ring);
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

	// 这一只环。内沿黑、外沿白写在构造里。
	std::unique_ptr<Ring_Shape> ring_;
	// 这一次播放的圆心。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// SpawnAt 传入的水平方向。现在只保存并传给下一只环，受击不读取。
	XMFLOAT3 hitDir_{ 0.0f, 0.0f, 0.0f };
	// 从 SpawnAt 起算的秒数。
	float age_{ 0.0f };
	// 上一帧外沿的世界半径。
	float prevOuter_{ 0.0f };
	// 这一帧外沿的世界半径。
	float outer_{ 0.0f };
	// 这次要扩到的世界半径。
	float targetRadius_{ 7.0f };
	// 打中后给予的阳标记个数。
	int markCount_{ 0 };
	// 传给下一只环的固定伤害。
	float areaDamage_{ 0.0f };
	// 这一次命中还剩几只环。同一轮放出去的环共用。
	std::shared_ptr<int> burstsLeft_;
	// 这只环已经打过的敌人。再次出生时清空。
	std::vector<EnemyBase*> hitEnemies_;
	// 池不够时用来再造下一只环。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 从 0 扩到目标半径用的秒数。
	static constexpr float kExpandSeconds_{ 0.35f };
	// 扩完之后淡出用的秒数。淡出时外沿不动。
	static constexpr float kFadeSeconds_{ 0.25f };
	// 出生时写回的目标半径。
	static constexpr float kDefaultRadius_{ 3.0f };
	// 满这么多个再放下一只环。
	static constexpr int kPerTrigger_{ 10 };
	// 受击时向上的速度。不加水平速度。正在上升时不超过这一次。和阳环相同。
	static constexpr float kKnockbackUp_{ 0.1f };
};
