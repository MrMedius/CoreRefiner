#pragma once
#include "IModule.h"
#include "AttackBase.h"
#include "EnemyBase.h"
#include "ExecuteSphere.h"
#include "Graphics.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

// 命中时给敌人加上阴标记，不消耗。标记数乘秒杀倍率大于当前血量时，把血打到 0，并在敌人位置放一只黑球。
// 黑球不收尸，不额外掉金币。死亡仍由敌人自己走。
class Module_Element_GreaterYin : public IModule
{
public:
	// markCount 这次命中加上的标记数。executeRate 是秒杀倍率。
	// gfx 和 rg 在池不够时用来再造一只黑球。
	Module_Element_GreaterYin(AttackBase* owner, float markCount, float executeRate, Graphics& gfx, Rgph::RenderGraph& rg) noexcept
		:
		IModule(owner),
		markCount_(static_cast<int>(markCount)),
		executeRate_(executeRate),
		gfx_(gfx),
		rg_(rg)
	{}

	// 只对敌人结算。先加上标记，再和扣血之后的当前血量比。成立就把血打到 0，并放黑球。不击退，不计统计，不调用 Trigger。
	void OnHit(CharacterBase* other) override
	{
		EnemyBase* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr)
		{
			return;
		}
		// 这次没有标记可加时，不改格子，也不拿原来的个数去秒杀。
		if (markCount_ <= 0)
		{
			return;
		}
		enemy->Marks().Add(AttackMark::Yin, markCount_);
		// 比的是加上之后格子里的个数，不是这次新加的那一点。同一发上的阴模块仍会自己按满 5 个 Trigger。
		const float marks = static_cast<float>(enemy->Marks().Get(AttackMark::Yin));
		if (marks * executeRate_ > enemy->GetHpCurrent())
		{
			FinishHp_(*enemy);
			SpawnSphere_(*enemy);
		}
	}

private:
	// 条件已经成立。血还大于 0 就扣到 0，让受伤状态走进死亡。血已经是 0 时不再扣。
	void FinishHp_(EnemyBase& enemy) const
	{
		enemy.SetIsHurt(true);
		const float hp = enemy.GetHpCurrent();
		if (hp > 0.0f)
		{
			enemy.CalculateHpCurrent(-hp);
		}
	}

	// 在敌人当前位置取出一只黑球。半径是碰撞体最长的整条尺寸。副本从身体外形记下网格、颜色、位置、旋转和缩放。
	// 池里没有就按 gfx、rg 再造，然后交给特效管理器认领。
	void SpawnSphere_(const EnemyBase& enemy) const
	{
		ExecuteSphere* fx = ObjectCodex::SpawnPooled<ExecuteSphere>(effect_ExecuteSphere, gfx_, rg_);
		if (fx == nullptr)
		{
			return;
		}
		const ColliderComponentBase* collider = enemy.GetComponent<ColliderComponentBase>();
		const float radius = collider != nullptr ? ExecuteSphereRadius(*collider) : 0.0f;
		fx->Play(enemy.GetWorldPosition(), radius, enemy.GetBodyDrawable());
		ObjectCodex::Handoff(effect_ExecuteSphere, fx);
	}

	// 这次命中要加上的标记个数。
	int markCount_{ 0 };
	// 秒杀倍率。格子里的个数乘它，大于当前血量就成立。
	float executeRate_{ 0.0f };
	// 对象池扩容时传给黑球。
	Graphics& gfx_;
	// 对象池扩容时传给黑球。
	Rgph::RenderGraph& rg_;
};
