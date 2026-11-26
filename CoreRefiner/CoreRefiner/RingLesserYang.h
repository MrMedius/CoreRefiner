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
// 不击退。环带外沿白、内沿黑，约 0.35 秒扩到目标半径。
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
		// 黑环整条不透明。白环外实内虚，后画，盖住外沿。
		MakeLayer_(ringBlack_, gfx, rg, { 0.0f, 0.0f, 0.0f }, false);
		MakeLayer_(ringWhite_, gfx, rg, { 1.0f, 1.0f, 1.0f }, true);
	}

	// pos 是圆心。dir 这一步不用，少阳环不击退。
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		(void)dir;
		origin_ = pos;
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
		if (ApplyRing_())
		{
			// 先画黑，再画白。透明通道不写深度，后画的白盖在外沿上。
			if (ringBlack_ != nullptr)
			{
				ringBlack_->Submit(Chan::main);
			}
			if (ringWhite_ != nullptr)
			{
				ringWhite_->Submit(Chan::main);
			}
		}
		ObjectBase::Submit();
	}

protected:
	// 扣这只环自己的固定伤害。同一个人只打一次，不击退。打中之后才加标记。
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

	// 在这个敌人脚下再放一只。次数先扣，池里没有再还回去。
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
		ring->SpawnAt(enemy.GetWorldPosition(), { 0.0f, 0.0f, 1.0f });
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

	// 造一层环。gradient 为真时外实内虚，为假时整条同样实。少阳环不起浪，避免内沿被波浪染白。
	static void MakeLayer_(std::unique_ptr<Ring_Shape>& ring, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 rgb, bool gradient)
	{
		ring = std::make_unique<Ring_Shape>(gfx);
		ring->LinkTechniques(rg);
		ring->SetColor(rgb);
		ring->SetGradient(gradient);
		ring->SetInnerWave(false);
		ring->SetOuterWave(false);
		ring->SetAmplitude(0.0f);
	}

	// 把这一层放到圆心，缩放到当前外沿，并写上整层透明度。
	void PlaceLayer_(Ring_Shape& ring, float opacity)
	{
		ring.SetPosition(origin_);
		ring.SetScale({ outer_, outer_, outer_ });
		ring.SetOpacity(opacity);
	}

	// 按年龄写下两层的位置、缩放和透明度。还在播放返回 true。
	bool ApplyRing_()
	{
		if (ringBlack_ == nullptr || ringWhite_ == nullptr || age_ <= 0.0f)
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
		PlaceLayer_(*ringBlack_, opacity);
		PlaceLayer_(*ringWhite_, opacity);
		return true;
	}

	// 垫在下面的黑环。整条环带不透明，白环变淡的地方露出它。
	std::unique_ptr<Ring_Shape> ringBlack_;
	// 盖在上面的白环。外沿实、内沿透，所以看起来外白内黑。
	std::unique_ptr<Ring_Shape> ringWhite_;
	// 这一次播放的圆心。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
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
	static constexpr float kDefaultRadius_{ 7.0f };
	// 满这么多个再放下一只环。
	static constexpr int kPerTrigger_{ 10 };
};
