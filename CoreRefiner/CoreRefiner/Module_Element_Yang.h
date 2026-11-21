#pragma once
#include "IModule.h"
#include "Attack.h"
#include "Ball.h"
#include "Enemy.h"
#include "Graphics.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

#include <cstdint>

// 命中时给敌人加阳标记。每满 5 个消耗一次，对范围内还活着的敌人各扣一次血。
// 白环已经改成攻击，这一步先不生成。
class Module_Element_Yang : public IModule
{
public:
	// markCount 这次命中加上的标记数。areaDamage 每触发一次、范围内每人扣的血，不吃加成。
	// radius 是水平半径。gfx 和 rg 留给以后生成攻击环，这一步先存着。
	Module_Element_Yang(Attack* owner, float markCount, float areaDamage, float radius, Graphics& gfx, Rgph::RenderGraph& rg) noexcept
		:
		IModule(owner),
		markCount_(static_cast<int>(markCount)),
		areaDamage_(areaDamage),
		radius_(radius),
		gfx_(gfx),
		rg_(rg)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Element_Yang;
	}

	// 给宿主球追加一条白色条纹。
	void OnSpawn() override
	{
		if (Ball* ball = dynamic_cast<Ball*>(GetOwner()))
		{
			ball->AppendColorId(kColorId_);
		}
	}

	// 只对敌人结算。触发几次就做几次范围伤害。别人不加标记，不击退，不计统计。
	void OnHit(Character* other) override
	{
		Enemy* enemy = dynamic_cast<Enemy*>(other);
		if (enemy == nullptr)
		{
			return;
		}
		const int triggers = enemy->Marks().Add(AttackMark::Yang, markCount_, kPerTrigger_);
		for (int i = 0; i < triggers; ++i)
		{
			ApplyAreaDamage_(*enemy);
		}
	}

private:
	// 以目标为圆心，按水平距离找还活着的敌人，每人扣一次血并进入受伤。目标自己也算在内，只进这一次。
	void ApplyAreaDamage_(const Enemy& center) const
	{
		const XMFLOAT3 origin = center.GetWorldPosition();
		const float reachSq = radius_ * radius_;
		for (Object_Type_Tag tag : kCharacterTags)
		{
			for (Enemy* enemy : ObjectCodex::FindActiveObjectsByTag<Enemy>(tag))
			{
				if (enemy == nullptr || enemy->GetIsDeath())
				{
					continue;
				}
				const XMFLOAT3 pos = enemy->GetWorldPosition();
				const float dx = pos.x - origin.x;
				const float dz = pos.z - origin.z;
				if (dx * dx + dz * dz > reachSq)
				{
					continue;
				}
				enemy->SetIsHurt(true);
				enemy->CalculateHpCurrent(-areaDamage_);
			}
		}
	}

	// 这次命中要加上的标记个数。
	int markCount_{ 0 };
	// 范围内每个还活着的敌人要扣的血量。
	float areaDamage_{ 0.0f };
	// 水平作用半径。比较时用它的平方。
	float radius_{ 0.0f };
	// 以后生成攻击环时用来扩池。这一步还不生成。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 满这么多个就消耗一次，余数留下。
	static constexpr int kPerTrigger_{ 5 };
	// 阳的条纹颜色编号，白色。
	static constexpr std::uint32_t kColorId_{ 1u };
};
