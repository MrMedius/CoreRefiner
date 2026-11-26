#pragma once
#include "IModule.h"
#include "AttackBase.h"
#include "EnemyBase.h"
#include "Graphics.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"
#include "RingLesserYang.h"

#include <cmath>
#include <memory>

// 命中时给敌人加阳标记。每满 10 个消耗一轮，就在目标脚下生成一只少阳环。
// 扫人、扣血和下一轮标记都由那只环做。
class Module_Element_LesserYang : public IModule
{
public:
	// markCount 是每次给予的阳标记个数。areaDamage 是每只环扣的固定血量。radius 是水平半径。
	// gfx 和 rg 在池不够时用来再造环。
	Module_Element_LesserYang(AttackBase* owner, float markCount, float areaDamage, float radius, Graphics& gfx, Rgph::RenderGraph& rg) noexcept
		:
		IModule(owner),
		markCount_(static_cast<int>(markCount)),
		areaDamage_(areaDamage),
		radius_(radius),
		gfx_(gfx),
		rg_(rg)
	{}

	// 只给被打中的敌人加标记。够一轮就放一只环，直到不够或次数用完。
	void OnHit(CharacterBase* other) override
	{
		EnemyBase* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr || markCount_ <= 0)
		{
			return;
		}
		enemy->Marks().Add(AttackMark::Yang, markCount_);
		auto burstsLeft = std::make_shared<int>(kMaxBursts_);
		while (*burstsLeft > 0 && enemy->Marks().Trigger(AttackMark::Yang, kPerTrigger_) > 0)
		{
			SpawnRing_(*enemy, burstsLeft);
		}
	}

private:
	// 取宿主当前的水平前进方向。长度接近 0 时用世界 +Z。只给站在环心上的人击退。
	XMFLOAT3 HorizontalForward_() const
	{
		XMFLOAT3 dir{ 0.0f, 0.0f, 1.0f };
		if (AttackBase* owner = GetOwner())
		{
			dir = owner->GetForward();
			dir.y = 0.0f;
		}
		const float lenSq = dir.x * dir.x + dir.z * dir.z;
		if (lenSq <= 1.0e-8f)
		{
			return { 0.0f, 0.0f, 1.0f };
		}
		const float inv = 1.0f / std::sqrt(lenSq);
		dir.x *= inv;
		dir.z *= inv;
		return dir;
	}

	// 在被打中的敌人脚下取出一只少阳环。伤害、半径和剩余次数写在出生之后。
	void SpawnRing_(const EnemyBase& enemy, const std::shared_ptr<int>& burstsLeft) const
	{
		if (burstsLeft == nullptr || *burstsLeft <= 0)
		{
			return;
		}
		*burstsLeft -= 1;
		RingLesserYang* ring = ObjectCodex::SpawnPooled<RingLesserYang>(attack_Ring_LesserYang, gfx_, rg_);
		if (ring == nullptr)
		{
			*burstsLeft += 1;
			return;
		}
		ring->SpawnAt(enemy.GetWorldPosition(), HorizontalForward_());
		ring->SetBurst(areaDamage_, radius_, markCount_, burstsLeft);
		ObjectCodex::Handoff(attack_Ring_LesserYang, ring);
	}

	// 每次给予的阳标记个数。
	int markCount_{ 0 };
	// 每只环扣的固定血量。
	float areaDamage_{ 0.0f };
	// 环要扩到的水平半径。
	float radius_{ 0.0f };
	// 对象池扩容时传给新的环。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 满这么多个就消耗一轮。
	static constexpr int kPerTrigger_{ 10 };
	// 同一次命中最多放这么多只环。放出去的环继续共用这个次数。
	static constexpr int kMaxBursts_{ 64 };
};
