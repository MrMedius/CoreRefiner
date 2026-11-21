#pragma once
#include "IModule.h"
#include "AttackBase.h"
#include "Ball.h"
#include "EnemyBase.h"
#include "Ring_Yang.h"
#include "Graphics.h"
#include "ObjectCodex.h"
#include "RenderGraph.h"

#include <cmath>
#include <cstdint>

// 命中时给敌人加阳标记。每满 5 个消耗一次，就在目标脚下生成一只白环。
// 周围的敌人不加标记。环自己再按波前扣血。
class Module_Element_Yang : public IModule
{
public:
	// markCount 这次命中加上的标记数。areaDamage 每触发一次写进白环的伤害，不吃加成。
	// radius 是白环要扩到的世界半径。gfx 和 rg 在池不够时用来再造一只环。
	Module_Element_Yang(AttackBase* owner, float markCount, float areaDamage, float radius, Graphics& gfx, Rgph::RenderGraph& rg) noexcept
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

	// 只给被子弹打中的敌人加标记。触发几次就生成几只环。
	void OnHit(CharacterBase* other) override
	{
		EnemyBase* enemy = dynamic_cast<EnemyBase*>(other);
		if (enemy == nullptr)
		{
			return;
		}
		const int triggers = enemy->Marks().Add(AttackMark::Yang, markCount_, kPerTrigger_);
		for (int i = 0; i < triggers; ++i)
		{
			SpawnRing_(*enemy);
		}
	}

private:
	// 取宿主当前的水平前进方向。长度接近 0 时用世界 +Z。
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

	// 在被打中的敌人脚下取出一只白环。伤害和半径写在出生之后，出生会把它们收回默认值。
	void SpawnRing_(const EnemyBase& enemy) const
	{
		Ring_Yang* ring = ObjectCodex::SpawnPooled<Ring_Yang>(attack_Ring_Yang, gfx_, rg_);
		if (ring == nullptr)
		{
			return;
		}
		ring->SpawnAt(enemy.GetWorldPosition(), HorizontalForward_());
		ring->Stats().damage.base = areaDamage_;
		ring->Stats().damage.ResetMods();
		ring->SetTargetRadius(radius_);
		// 交给攻击管理器认领。这一帧的攻击更新已经走过，下一帧才进名单。
		ObjectCodex::Handoff(attack_Ring_Yang, ring);
	}

	// 这次命中要加上的标记个数。
	int markCount_{ 0 };
	// 每触发一次写进白环的伤害。
	float areaDamage_{ 0.0f };
	// 白环要扩到的世界半径。
	float radius_{ 0.0f };
	// 对象池扩容时传给新的白环。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 满这么多个就消耗一次，余数留下。
	static constexpr int kPerTrigger_{ 5 };
	// 阳的条纹颜色编号，白色。
	static constexpr std::uint32_t kColorId_{ 1u };
};
