#pragma once
#include "IModule.h"
#include "AttackBase.h"
#include "Ball.h"
#include "EnemyBase.h"
#include "RingYin.h"
#include "Graphics.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

#include <cstdint>

// 命中时给敌人加阴标记。每满 5 个消耗一次，扣一次额外伤害，并在目标处放一只黑环。
class Module_Element_Yin : public IModule
{
public:
	// markCount 这次命中加上的标记数。extraDamage 每触发一次扣的血，不吃加成。
	// gfx 和 rg 在池不够时用来再造一只阴环。
	Module_Element_Yin(AttackBase* owner, float markCount, float extraDamage, Graphics& gfx, Rgph::RenderGraph& rg) noexcept
		:
		IModule(owner),
		markCount_(static_cast<int>(markCount)),
		extraDamage_(extraDamage),
		gfx_(gfx),
		rg_(rg)
	{}

	// 给宿主球追加一条黑色条纹。
	void OnSpawn() override
	{
		if (Ball* ball = dynamic_cast<Ball*>(GetOwner()))
		{
			ball->AppendColorId(kColorId_);
		}
	}

	// 只对敌人结算。触发几次就扣几次血、放几只环。不击退，不计统计。
	void OnHit(CharacterBase* other) override
	{
		EnemyBase* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr)
		{
			return;
		}
		// 这次没有标记可加时，不消耗格子里已经有的个数。
		if (markCount_ <= 0)
		{
			return;
		}
		enemy->Marks().Add(AttackMark::Yin, markCount_);
		// Trigger 一次只扣满 5 个的一轮。返回 1 就扣一次血、放一只环，直到不够一轮。
		while (enemy->Marks().Trigger(AttackMark::Yin, kPerTrigger_) > 0)
		{
			ApplyExtraDamage_(*enemy);
			SpawnEffect_(*enemy);
		}
	}

private:
	// 扣这次触发的额外伤害，并进入受伤，血量归零才能走进死亡。
	void ApplyExtraDamage_(EnemyBase& enemy) const
	{
		enemy.SetIsHurt(true);
		enemy.CalculateHpCurrent(-extraDamage_);
	}

	// 在敌人当前位置取出一只阴环开始播放，并交给特效管理器认领。池里没有就按 gfx、rg 再造。
	void SpawnEffect_(const EnemyBase& enemy) const
	{
		if (RingYin* fx = ObjectCodex::SpawnPooled<RingYin>(effect_Ring_Yin, gfx_, rg_))
		{
			fx->SpawnAt(enemy.GetWorldPosition(), false);
			ObjectCodex::Handoff(effect_Ring_Yin, fx);
		}
	}

	// 这次命中要加上的标记个数。
	int markCount_{ 0 };
	// 每触发一次扣掉的血量。
	float extraDamage_{ 0.0f };
	// 对象池扩容时传给新特效。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 满这么多个就消耗一次，余数留下。
	static constexpr int kPerTrigger_{ 5 };
	// 阴的条纹颜色编号，黑色。
	static constexpr std::uint32_t kColorId_{ 2u };
};
