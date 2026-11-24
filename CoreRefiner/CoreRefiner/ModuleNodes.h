#pragma once
#include "IModuleNode.h"
#include "AttackNodeSteps.h"
#include "Canvas2DSpriteUV.h"
#include "Graphics.h"
#include "GameRulesCodex.h"
#include "IconAtlas.h"
#include "RenderGraph.h"
#include "TimeCodex.h"
#include "Util.h"

#include <cmath>

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

// ————————————————————————————————————————————————————
// Kind —— Core
// ————————————————————————————————————————————————————
class ModuleNode_Core_Ball final : public IModuleNode
{
public:
	explicit ModuleNode_Core_Ball(
		DirectX::XMFLOAT2 localPos = { 0.0f, 0.0f },
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		IModuleNode(ModuleNodeLabel::Core_Ball),
		scale_(scale),
		enableCollider_(enableCollider)
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(100.0f);
		SetScanExpandSpeed(100.0f);
		SetBuyPrice(0);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Core_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

protected:
	void ApplyLevelStats_() override
	{
		hitRadius_ = kHitRadius_[level_.Index()];
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 20.0f, 17.0f, 14.0f };
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 2.0f, 1.5f, 1.0f };
};

// ————————————————————————————————————————————————————
// Kind —— Spawn
// ————————————————————————————————————————————————————
class ModuleNode_Spawn_Ball final : public IModuleNode
{
public:
	explicit ModuleNode_Spawn_Ball(
		DirectX::XMFLOAT2 localPos,
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		IModuleNode(ModuleNodeLabel::Spawn_Ball),
		scale_(scale),
		enableCollider_(enableCollider)
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(80.0f);
		SetScanExpandSpeed(80.0f);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Spawn_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

protected:
	void ApplyLevelStats_() override
	{
		hitRadius_ = kHitRadius_[level_.Index()];
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 24.0f, 21.0f, 18.0f };
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 2.0f, 1.5f, 1.0f };
};

// ————————————————————————————————————————————————————
// Kind —— Attribute
// ————————————————————————————————————————————————————
class ModuleNode_Attribute_LifetimeRate final : public IModuleNode
{
public:
	explicit ModuleNode_Attribute_LifetimeRate(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Attribute_LifetimeRate)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(100.0f);
		lifetimeRate_ = kLifetimeRate_[0];
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_LifetimeRate::Make(lifetimeRate_)->Apply(ctx);
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::LifetimeRate, lifetimeRate_ });
	}

protected:
	void ApplyLevelStats_() override
	{
		lifetimeRate_ = kLifetimeRate_[level_.Index()];
	}

private:
	float lifetimeRate_{ 0.5f };
	static constexpr float kLifetimeRate_[ModuleNodeLevel::kCount] = { 0.5f, 1.0f, 1.5f };
};

class ModuleNode_Attribute_SpeedRate final : public IModuleNode
{
public:
	explicit ModuleNode_Attribute_SpeedRate(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Attribute_SpeedRate)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(100.0f);
		speedRate_ = kSpeedRate_[0];
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_SpeedRate::Make(speedRate_)->Apply(ctx);
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::SpeedRate, speedRate_ });
	}

protected:
	void ApplyLevelStats_() override
	{
		speedRate_ = kSpeedRate_[level_.Index()];
	}

private:
	float speedRate_{ 0.5f };
	static constexpr float kSpeedRate_[ModuleNodeLevel::kCount] = { 0.5f, 1.0f, 1.5f };
};

class ModuleNode_Attribute_SizeRate final : public IModuleNode
{
public:
	explicit ModuleNode_Attribute_SizeRate(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Attribute_SizeRate)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(100.0f);
		sizeRate_ = kSizeRate_[0];
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_SizeRate::Make(sizeRate_)->Apply(ctx);
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::SizeRate, sizeRate_ });
	}

protected:
	void ApplyLevelStats_() override
	{
		sizeRate_ = kSizeRate_[level_.Index()];
	}

private:
	float sizeRate_{ 0.5f };
	static constexpr float kSizeRate_[ModuleNodeLevel::kCount] = { 0.5f, 1.0f, 1.5f };
};

class ModuleNode_Attribute_DamageRate final : public IModuleNode
{
public:
	explicit ModuleNode_Attribute_DamageRate(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Attribute_DamageRate)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(100.0f);
		damageRate_ = kDamageRate_[0];
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_DamageRate::Make(damageRate_)->Apply(ctx);
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::DamageRate, damageRate_ });
	}

protected:
	void ApplyLevelStats_() override
	{
		damageRate_ = kDamageRate_[level_.Index()];
	}

private:
	float damageRate_{ 0.5f };
	static constexpr float kDamageRate_[ModuleNodeLevel::kCount] = { 0.5f, 1.0f, 1.5f };
};

// ————————————————————————————————————————————————————
// Kind —— Rule
// ————————————————————————————————————————————————————
class ModuleNode_Rule_Orbit final : public IModuleNode
{
public:
	ModuleNode_Rule_Orbit(
		DirectX::XMFLOAT2 localPos,
		float radius = 2.0f,
		float phase = -1.0f) noexcept
		:
		IModuleNode(ModuleNodeLabel::Rule_Orbit),
		orbitRadius_(radius),
		orbitPhase_(phase)
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		SetCooldownDuration(1.0f);
		SetScanMaxRadius(80.0f);
		SetScanExpandSpeed(80.0f);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Rule_Orbit::Make(orbitRadius_, orbitPhase_)->Apply(ctx);
	}

protected:
	void ApplyLevelStats_() override
	{
		hitRadius_ = kHitRadius_[level_.Index()];
	}

private:
	float orbitRadius_{ 2.0f };
	float orbitPhase_{ -1.0f };
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 18.0f, 14.0f, 10.0f };
};

class ModuleNode_Rule_Return final : public IModuleNode
{
public:
	explicit ModuleNode_Rule_Return(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Rule_Return)
	{
		localPos_ = localPos;
		hitRadius_ = 15.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(kscanMaxRadius_[0]);
		SetScanExpandSpeed(kscanExpandSpeed_[0]);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Rule_Return::Make()->Apply(ctx);
	}

protected:
	void ApplyLevelStats_() override
	{
		SetScanMaxRadius(kscanMaxRadius_[level_.Index()]);
		SetScanExpandSpeed(kscanExpandSpeed_[level_.Index()]);
	}

private:
	static constexpr float kscanMaxRadius_[ModuleNodeLevel::kCount] = { 50.0f, 60.0f, 70.0f };
	static constexpr float kscanExpandSpeed_[ModuleNodeLevel::kCount] = { 50.0f, 75.0f, 100.0f };
};

class ModuleNode_Rule_Child final : public IModuleNode
{
public:
	explicit ModuleNode_Rule_Child(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Rule_Child)
	{
		localPos_ = localPos;
		hitRadius_ = 20.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(80.0f);
		SetScanExpandSpeed(kscanExpandSpeed_[0]);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Rule_Child::Make()->Apply(ctx);
	}

protected:
	void ApplyLevelStats_() override
	{
		SetScanExpandSpeed(kscanExpandSpeed_[level_.Index()]);
	}

private:
	static constexpr float kscanExpandSpeed_[ModuleNodeLevel::kCount] = { 50.0f, 75.0f, 100.0f };
};

class ModuleNode_Rule_Revive final : public IModuleNode
{
public:
	explicit ModuleNode_Rule_Revive(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Rule_Revive)
	{
		localPos_ = localPos;
		hitRadius_ = 15.0f;
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(100.0f);
		SetScanExpandSpeed(100.0f);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Rule_Revive::Make()->Apply(ctx);
	}

protected:
	void ApplyLevelStats_() override
	{
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 5.0f, 4.0f, 3.0f };
};

// ————————————————————————————————————————————————————
// Kind —— Element
// ————————————————————————————————————————————————————
// 阴。扫到后挂到当前聚焦的那发弹上。造价用默认 4。
class ModuleNode_Element_Yin final : public IModuleNode
{
public:
	explicit ModuleNode_Element_Yin(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Element_Yin)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		markCount_ = kMarkCount_[0];
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(50.0f);
	}

	// 把当前等级的标记数和固定隐藏伤害交给阴步骤。
	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Element_Yin::Make(markCount_, extraDamage_)->Apply(ctx);
	}

	// 面板只提交标记数。隐藏伤害不进说明书。
	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::YinMarkCount, markCount_ });
	}

protected:
	// 按等级改标记数和冷却。半径和扩散速度不变。
	void ApplyLevelStats_() override
	{
		markCount_ = kMarkCount_[level_.Index()];
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	// 这一级每次命中加上的标记个数。
	float markCount_{ 1.0f };
	// 每触发一次扣的隐藏伤害。不随等级变。
	float extraDamage_{ kExtraDamage_ };
	// 1、2、3 级的标记数。
	static constexpr float kMarkCount_[ModuleNodeLevel::kCount] = { 1.0f, 2.0f, 3.0f };
	// 1、2、3 级的冷却秒数。
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 5.0f, 3.5f, 2.0f };
	// 每满一次标记扣的血。
	static constexpr float kExtraDamage_{ 2.0f };
};

// 阳。扫到后挂到当前聚焦的那发弹上。半径、扩散、冷却和造价与阴相同。
class ModuleNode_Element_Yang final : public IModuleNode
{
public:
	explicit ModuleNode_Element_Yang(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Element_Yang)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		markCount_ = kMarkCount_[0];
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(50.0f);
	}

	// 把当前等级的标记数、固定范围伤害和环半径交给阳步骤。
	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Element_Yang::Make(markCount_, areaDamage_, radius_)->Apply(ctx);
	}

	// 面板只提交标记数。范围伤害和半径不进说明书。
	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::YangMarkCount, markCount_ });
	}

protected:
	// 按等级改标记数和冷却。半径和扩散速度不变。
	void ApplyLevelStats_() override
	{
		markCount_ = kMarkCount_[level_.Index()];
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	// 这一级每次命中加上的标记个数。
	float markCount_{ 1.0f };
	// 白环外沿碰到的每个敌人要扣的血。不随等级变。
	float areaDamage_{ kAreaDamage_ };
	// 白环要扩到的世界半径。不随等级变。
	float radius_{ kRadius_ };
	// 1、2、3 级的标记数。
	static constexpr float kMarkCount_[ModuleNodeLevel::kCount] = { 1.0f, 2.0f, 3.0f };
	// 1、2、3 级的冷却秒数。与阴相同。
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 5.0f, 3.5f, 2.0f };
	// 每满一次标记，环上每个被外沿打到的敌人扣的血。
	static constexpr float kAreaDamage_{ 0.5f };
	// 白环的世界半径。
	static constexpr float kRadius_{ 10.0f };
};

// 太阴。扫到后挂到当前聚焦的那发弹上。造价用默认 4。这一步还不挂攻击。
class ModuleNode_Element_GreaterYin final : public IModuleNode
{
public:
	explicit ModuleNode_Element_GreaterYin(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Element_GreaterYin)
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		markCount_ = kMarkCount_[0];
		executeRate_ = kExecuteRate_[0];
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(kScanMaxRadius_);
		SetScanExpandSpeed(kScanExpandSpeed_);
	}

	// 配方扫到时先什么都不做。命中结算以后再挂上。
	void ApplyTo(DeployContext& ctx) override
	{
		(void)ctx;
	}

	// 面板提交给予的阴标记个数和秒杀倍率。
	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::YinMarkCount, markCount_ });
		out.push_back({ ModuleNodeStat::YinExecuteRate, executeRate_ });
	}

protected:
	// 按等级改节点半径、冷却、给予个数和秒杀倍率。扩散半径和速度不变。
	void ApplyLevelStats_() override
	{
		hitRadius_ = kHitRadius_[level_.Index()];
		markCount_ = kMarkCount_[level_.Index()];
		executeRate_ = kExecuteRate_[level_.Index()];
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	// 这一级每次命中加上的阴标记个数。
	float markCount_{ 1.0f };
	// 这一级的秒杀倍率。标记数乘它，再和比较血量比。
	float executeRate_{ 1.2f };
	// 1、2、3 级的节点半径。
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 30.0f, 27.0f, 24.0f };
	// 1、2、3 级的冷却秒数。
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 8.0f, 6.0f, 4.0f };
	// 1、2、3 级的给予阴标记个数。
	static constexpr float kMarkCount_[ModuleNodeLevel::kCount] = { 1.0f, 1.0f, 2.0f };
	// 1、2、3 级的秒杀倍率。
	static constexpr float kExecuteRate_[ModuleNodeLevel::kCount] = { 1.2f, 1.6f, 2.0f };
	// 扫描扩到的世界半径。不随等级变。
	static constexpr float kScanMaxRadius_{ 20.0f };
	// 扫描向外扩的速度。不随等级变。
	static constexpr float kScanExpandSpeed_{ 20.0f };
};

// 少阳。扫到后挂到当前聚焦的那发弹上。造价用默认 4。这一步还不挂攻击。
class ModuleNode_Element_LesserYang final : public IModuleNode
{
public:
	explicit ModuleNode_Element_LesserYang(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Element_LesserYang)
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		markCount_ = kMarkCount_[0];
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(kScanMaxRadius_);
		SetScanExpandSpeed(kScanExpandSpeed_);
	}

	// 配方扫到时先什么都不做。范围结算以后再挂上。
	void ApplyTo(DeployContext& ctx) override
	{
		(void)ctx;
	}

	// 面板只提交给予的阳标记个数。范围固定伤害和波及半径不进说明书。
	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::YangMarkCount, markCount_ });
	}

protected:
	// 按等级改节点半径、冷却和给予个数。扩散半径和速度不变。
	void ApplyLevelStats_() override
	{
		hitRadius_ = kHitRadius_[level_.Index()];
		markCount_ = kMarkCount_[level_.Index()];
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	// 这一级每次命中加上的阳标记个数。
	float markCount_{ 1.0f };
	// 每次范围爆发扣的固定伤害。不吃攻击加成，不随等级变，不进面板。
	float areaDamage_{ kAreaDamage_ };
	// 范围爆发的水平半径。不随等级变，不进面板。
	float burstRadius_{ kBurstRadius_ };
	// 1、2、3 级的节点半径。
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 22.0f, 20.0f, 16.0f };
	// 1、2、3 级的冷却秒数。
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 6.0f, 4.0f, 2.0f };
	// 1、2、3 级的给予阳标记个数。
	static constexpr float kMarkCount_[ModuleNodeLevel::kCount] = { 1.0f, 2.0f, 4.0f };
	// 每次范围爆发扣的血。
	static constexpr float kAreaDamage_{ 0.5f };
	// 范围爆发的世界半径。
	static constexpr float kBurstRadius_{ 7.0f };
	// 扫描扩到的世界半径。不随等级变。
	static constexpr float kScanMaxRadius_{ 40.0f };
	// 扫描向外扩的速度。不随等级变。
	static constexpr float kScanExpandSpeed_{ 40.0f };
};

// ————————————————————————————————————————————————————
// Kind —— Passive
// ————————————————————————————————————————————————————
class ModuleNode_Passive_DamageFix final : public IModuleNode
{
public:
	explicit ModuleNode_Passive_DamageFix(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Passive_DamageFix)
	{
		localPos_ = localPos;
		hitRadius_ = 10.0f;
		SetCooldownDuration(3.0f);
		SetScanMaxRadius(10.0f);
		SetScanExpandSpeed(10.0f);
		damageFix_ = kDamageFix_[0];
	}

	void ApplyTo(DeployContext& ctx) override
	{
		(void)ctx;
	}

	void ApplyWarehouseBonus(AttackBase& attack) override
	{
		attack.Stats().damage.fix += damageFix_;
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::DamageFix, damageFix_ });
	}

protected:
	void ApplyLevelStats_() override
	{
		damageFix_ = kDamageFix_[level_.Index()];
	}

private:
	float damageFix_{ 1.0f };
	static constexpr float kDamageFix_[ModuleNodeLevel::kCount] = { 1.0f, 1.5f, 2.0f };
};

// ————————————————————————————————————————————————————
// Kind —— Other
// ————————————————————————————————————————————————————
class ModuleNode_Other_Repeat final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Repeat(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Other_Repeat)
	{
		localPos_ = localPos;
		hitRadius_ = 15.0f;
		SetCooldownDuration(3.0f);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(50.0f);
		repeatCount_ = kRepeatCount_[0];
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeStat::RepeatCount, repeatCount_ });
	}

	void ApplyTo(DeployContext& ctx) override
	{
		// 按 Node 组取，不按记录条数。当前组（含 Fusion 里刚写上的主体）不参与。
		const std::size_t groupCount = ctx.recipeGroupStarts.size();
		if (groupCount == 0u)
		{
			return;
		}

		std::size_t take = static_cast<std::size_t>(repeatCount_);
		if (take < 1u)
		{
			take = 1u;
		}
		else if (take > 3u)
		{
			take = 3u;
		}
		if (take > groupCount)
		{
			take = groupCount;
		}

		// 先按组拷贝。重放时的 Record 和 Flush 会改配方，不能边读边改。
		const std::size_t firstGroup = groupCount - take;
		// 每个元素是一个已封口的节点组。空组不放进来。
		std::vector<std::vector<AttackStepRecord>> copiedGroups;
		for (std::size_t g = firstGroup; g < groupCount; ++g)
		{
			const std::size_t begin = ctx.recipeGroupStarts[g];
			const std::size_t end = (g + 1u < groupCount)
				? ctx.recipeGroupStarts[g + 1u]
				: ctx.recipeGroupStart;
			if (begin > end || end > ctx.recipe.size())
			{
				continue;
			}
			copiedGroups.emplace_back(
				ctx.recipe.begin() + static_cast<std::ptrdiff_t>(begin),
				ctx.recipe.begin() + static_cast<std::ptrdiff_t>(end));
		}

		// 复活已经打开记账。整段只再写进配方，不把生成改回当场发射。
		// 按原来的组切开，后面的重复仍按组数取。
		if (ctx.recordOnly)
		{
			bool opened = false;
			for (const std::vector<AttackStepRecord>& group : copiedGroups)
			{
				if (group.empty())
				{
					continue;
				}
				if (opened)
				{
					ctx.BeginRecipeGroup();
				}
				opened = true;
				for (const AttackStepRecord& rec : group)
				{
					ApplyAttackStepRecord(ctx, rec);
				}
			}
			return;
		}

		// 还没记账。核心和生成照旧当场发射。
		// 窗口里若有步骤把记账打开，后面的核心和生成仍要发射；没发射则把记账恢复。
		// 新分组只在这次从记账里发射之后重开。
		std::vector<AttackStepRecord> window;
		for (const std::vector<AttackStepRecord>& group : copiedGroups)
		{
			window.insert(window.end(), group.begin(), group.end());
		}
		const bool recordOnlyBefore = ctx.recordOnly;
		bool fired = false;
		for (const AttackStepRecord& rec : window)
		{
			// 核心和生成都要能发射。以后新增的 Label 只要进了这两张 Kind 表就算上。
			const ModuleNodeKind kind = KindOf(rec.label);
			const bool isCoreOrSpawn =
				kind == ModuleNodeKind::Core || kind == ModuleNodeKind::Spawn;
			if (isCoreOrSpawn)
			{
				ctx.recordOnly = false;
			}
			if (fired && recordOnlyBefore)
			{
				ctx.BeginRecipeGroup();
			}
			const std::size_t shotsNow = ctx.shots.size();
			ApplyAttackStepRecord(ctx, rec);
			if (ctx.shots.size() != shotsNow)
			{
				fired = true;
			}
		}
		if (!fired)
		{
			ctx.recordOnly = recordOnlyBefore;
		}
	}

protected:
	void ApplyLevelStats_() override
	{
		repeatCount_ = kRepeatCount_[level_.Index()];
	}

private:
	float repeatCount_{ 1.0f };
	static constexpr float kRepeatCount_[ModuleNodeLevel::kCount] = { 1.0f, 2.0f, 3.0f };
};

// ————————————————————————————————————————————————————
// Kind —— Fusion
// ————————————————————————————————————————————————————
class ModuleNode_Fusion final : public IModuleNode
{
public:
	ModuleNode_Fusion(std::unique_ptr<IModuleNode> primary, std::unique_ptr<IModuleNode> material, DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Fusion),
		primary_(std::move(primary)),
		material_(std::move(material))
	{
		localPos_ = localPos;
		if (primary_ != nullptr && material_ != nullptr)
		{
			SetHitRadius((primary_->GetHitRadius() + material_->GetHitRadius()) * 0.5f);
			SetCooldownDuration((primary_->GetCooldownDuration() + material_->GetCooldownDuration()) * 0.5f);
			SetScanMaxRadius((primary_->GetScanMaxRadius() + material_->GetScanMaxRadius()) * 0.5f);
			SetScanExpandSpeed((primary_->GetScanExpandSpeed() + material_->GetScanExpandSpeed()) * 0.5f);
			SetBuyPrice(primary_->GetBuyPrice() + material_->GetBuyPrice());
		}
		SetLevel(3);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		if (primary_ != nullptr)
		{
			primary_->ApplyTo(ctx);
		}
		if (material_ != nullptr)
		{
			material_->ApplyTo(ctx);
		}
	}

	void ApplyWarehouseBonus(AttackBase& attack) override
	{
		if (primary_ != nullptr)
		{
			primary_->ApplyWarehouseBonus(attack);
		}
		if (material_ != nullptr)
		{
			material_->ApplyWarehouseBonus(attack);
		}
	}

	[[nodiscard]] bool HasKind(ModuleNodeKind k) const noexcept override
	{
		if (GetKind() == k)
		{
			return true;
		}
		if (primary_ != nullptr && primary_->HasKind(k))
		{
			return true;
		}
		if (material_ != nullptr && material_->HasKind(k))
		{
			return true;
		}
		return false;
	}

	[[nodiscard]] IModuleNode* GetPrimary() noexcept
	{
		return primary_.get();
	}

	[[nodiscard]] const IModuleNode* GetPrimary() const noexcept
	{
		return primary_.get();
	}

	[[nodiscard]] IModuleNode* GetMaterial() noexcept
	{
		return material_.get();
	}

	[[nodiscard]] const IModuleNode* GetMaterial() const noexcept
	{
		return material_.get();
	}

	void BlitIcon(Canvas& canvas, bool asMask) const override
	{
		BlitHalves_(canvas, asMask);
	}

private:
	[[nodiscard]] static Color KindFillOf_(const IModuleNode& node) noexcept
	{
		const std::size_t i = ToIndex(node.GetKind());
		if (i >= ModuleNodeKindCount())
		{
			return ModuleNodeKindFill::kFill[ToIndex(ModuleNodeKind::Other)];
		}
		return ModuleNodeKindFill::kFill[i];
	}

	void BlitHalves_(Canvas& canvas, bool asMask) const
	{
		constexpr unsigned kMid = 8u;
		constexpr unsigned kEnd = 16u;
		const Color maskColor(0u, 0u, 0u, 160u);

		if (primary_ != nullptr)
		{
			IconAtlas::BlitIcon(canvas, NodeIconAtlas::Get(primary_->GetModuleNodeLabel()), asMask ? maskColor : KindFillOf_(*primary_), 0u, kMid);
		}
		if (material_ != nullptr)
		{
			IconAtlas::BlitIcon(canvas, NodeIconAtlas::Get(material_->GetModuleNodeLabel()), asMask ? maskColor : KindFillOf_(*material_), kMid, kEnd);
		}
	}

	std::unique_ptr<IModuleNode> primary_;
	std::unique_ptr<IModuleNode> material_;
};

// ————————————————————————————————————————————————————
// Kind —— Ultra
// ————————————————————————————————————————————————————
class ModuleNode_Ultra final : public IModuleNode
{
public:
	explicit ModuleNode_Ultra(DirectX::XMFLOAT2 localPos) noexcept
		:
		IModuleNode(ModuleNodeLabel::Ultra)
	{
		localPos_ = localPos;
		slots_.resize(static_cast<std::size_t>(GameRulesCodex::UltraBornCapacity));
		iconBits_ = NodeIconAtlas::detail::kRoundFrame;
		SetBuyPrice(0);
		SetLevel(3);
		RecomputeStats_();
	}

	void InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 zoneOrigin) override
	{
		zoneOrigin_ = zoneOrigin;

		// 16 帧斜向彩虹竖排。UV 先停在第 0 帧，和静止彩虹一致。
		auto sheet = std::make_unique<Canvas2DSpriteUV>(gfx, kVisualSize, kVisualSize * GameRulesCodex::RainbowFrames);
		sheet->Clear(Colors::None);
		IconAtlas::BakeRainbowSheet(*sheet, iconBits_, GameRulesCodex::RainbowFrames);
		sheet->NotifyPixelsChanged();
		sheet->LinkTechniques(rg);
		sheet->SetUVOffset(0.0f, 0.0f);
		sheet->SetUVScale(1.0f, 1.0f);
		sheet->SetSampleScale(1.0f, 1.0f / static_cast<float>(GameRulesCodex::RainbowFrames));
		icon_ = std::move(sheet);

		mask_ = std::make_unique<Canvas2DSpriteUV>(gfx, kVisualSize, kVisualSize);
		mask_->Clear(Colors::None);
		BlitIcon(*mask_, true);
		mask_->NotifyPixelsChanged();
		mask_->LinkTechniques(rg);
		mask_->SetUVOffset(0.0f, 0.0f);
		mask_->SetUVScale(1.0f, 0.0f);

		visualReady_ = true;
		SyncMaskUV_();
		ApplyVisualTransform_();
	}

	// 重绘图案后重烘焙整图和冷却遮罩。
	void RebakeIcon_()
	{
		if (icon_ != nullptr)
		{
			icon_->Clear(Colors::None);
			IconAtlas::BakeRainbowSheet(*icon_, iconBits_, GameRulesCodex::RainbowFrames);
			icon_->NotifyPixelsChanged();
		}
		if (mask_ != nullptr)
		{
			mask_->Clear(Colors::None);
			BlitIcon(*mask_, true);
			mask_->NotifyPixelsChanged();
		}
	}

	[[nodiscard]] int GetCapacity() const noexcept
	{
		return static_cast<int>(slots_.size());
	}

	void AddCapacity()
	{
		slots_.push_back(nullptr);
		Touch_();
	}

	[[nodiscard]] bool CanHold(const IModuleNode& node) const noexcept
	{
		const ModuleNodeKind kind = node.GetKind();
		return kind != ModuleNodeKind::Core && kind != ModuleNodeKind::Ultra;
	}

	[[nodiscard]] bool TryPut(std::size_t index, std::unique_ptr<IModuleNode>& node)
	{
		if (node == nullptr || index >= slots_.size() || slots_[index] != nullptr || !CanHold(*node))
		{
			return false;
		}
		slots_[index] = std::move(node);
		RecomputeStats_();
		Touch_();
		return true;
	}

	[[nodiscard]] std::unique_ptr<IModuleNode> Take(std::size_t index)
	{
		if (index >= slots_.size() || slots_[index] == nullptr)
		{
			return nullptr;
		}
		std::unique_ptr<IModuleNode> taken = std::move(slots_[index]);
		RecomputeStats_();
		Touch_();
		return taken;
	}

	[[nodiscard]] std::unique_ptr<IModuleNode> Take(IModuleNode* node)
	{
		if (node == nullptr)
		{
			return nullptr;
		}
		for (std::size_t i = 0; i < slots_.size(); ++i)
		{
			if (slots_[i].get() == node)
			{
				return Take(i);
			}
		}
		return nullptr;
	}

	[[nodiscard]] IModuleNode* GetSlot(std::size_t index) noexcept
	{
		return (index < slots_.size()) ? slots_[index].get() : nullptr;
	}

	[[nodiscard]] const IModuleNode* GetSlot(std::size_t index) const noexcept
	{
		return (index < slots_.size()) ? slots_[index].get() : nullptr;
	}

	[[nodiscard]] const std::string& GetName() const noexcept
	{
		return name_;
	}

	void SetName(std::string name)
	{
		if (Utf8CodepointCount(name) > GameRulesCodex::UltraNameMaxCodepoints)
		{
			std::size_t end = 0;
			for (std::size_t n = 0; n < GameRulesCodex::UltraNameMaxCodepoints && end < name.size(); ++n)
			{
				end = Utf8Next(name, end);
			}
			name.resize(end);
		}
		if (name_ == name)
		{
			return;
		}
		name_ = std::move(name);
		Touch_();
	}

	[[nodiscard]] const IconAtlas::IconBits& GetIconBits() const noexcept
	{
		return iconBits_;
	}

	void SetIconBits(const IconAtlas::IconBits& bits)
	{
		if (iconBits_ == bits)
		{
			return;
		}
		iconBits_ = bits;
		Touch_();
	}

	void ApplyTo(DeployContext& ctx) override
	{
		for (const std::unique_ptr<IModuleNode>& slot : slots_)
		{
			if (slot != nullptr)
			{
				slot->ApplyTo(ctx);
			}
		}
	}

	void ApplyWarehouseBonus(AttackBase& attack) override
	{
		for (const std::unique_ptr<IModuleNode>& slot : slots_)
		{
			if (slot != nullptr)
			{
				slot->ApplyWarehouseBonus(attack);
			}
		}
	}

	[[nodiscard]] bool HasKind(ModuleNodeKind k) const noexcept override
	{
		if (GetKind() == k)
		{
			return true;
		}
		for (const std::unique_ptr<IModuleNode>& slot : slots_)
		{
			if (slot != nullptr && slot->HasKind(k))
			{
				return true;
			}
		}
		return false;
	}

	// 名字、图案、容量、栏位变化次数。Panel 用它判断要不要重画，不进原来的等级/造价缓存。
	[[nodiscard]] std::uint32_t GetContentRevision() const noexcept
	{
		return contentRevision_;
	}

	void SubmitIcon() override
	{
		// 按全局时间切斜向彩虹。V 每次挪一帧高度，U 保持整宽。
		if (auto* sheet = dynamic_cast<Canvas2DSpriteUV*>(icon_.get()))
		{
			const float cycle = std::fmod(TimeCodex::Get().GetTotalTime(), 1.0f);
			const unsigned frame = static_cast<unsigned>(cycle * static_cast<float>(GameRulesCodex::RainbowFrames)) % GameRulesCodex::RainbowFrames;
			sheet->SetUVOffset(0.0f, static_cast<float>(frame) / static_cast<float>(GameRulesCodex::RainbowFrames));
		}
		IModuleNode::SubmitIcon();
	}

	void BlitIcon(Canvas& canvas, bool asMask) const override
	{
		if (asMask)
		{
			IconAtlas::BlitIcon(canvas, iconBits_, Color(0u, 0u, 0u, 160u));
			return;
		}
		// 静止斜向彩虹。流动留到切帧。
		IconAtlas::BlitIconRainbow(canvas, iconBits_, 0.0f);
	}

protected:
	void ApplyLevelStats_() override {}

private:
	void Touch_() noexcept
	{
		++contentRevision_;
	}

	// 有内容时四项取普通平均、造价求和；空奥义四项为 10、造价为 0。
	void RecomputeStats_()
	{
		float hit = 0.0f;
		float cooldown = 0.0f;
		float scanRadius = 0.0f;
		float scanSpeed = 0.0f;
		int price = 0;
		std::size_t count = 0;
		for (const std::unique_ptr<IModuleNode>& slot : slots_)
		{
			if (slot == nullptr)
			{
				continue;
			}
			hit += slot->GetHitRadius();
			cooldown += slot->GetCooldownDuration();
			scanRadius += slot->GetScanMaxRadius();
			scanSpeed += slot->GetScanExpandSpeed();
			price += slot->GetBuyPrice();
			++count;
		}
		if (count == 0u)
		{
			SetHitRadius(GameRulesCodex::DefaultStat);
			SetCooldownDuration(GameRulesCodex::DefaultStat);
			SetScanMaxRadius(GameRulesCodex::DefaultStat);
			SetScanExpandSpeed(GameRulesCodex::DefaultStat);
			SetBuyPrice(0);
			return;
		}
		const float inv = 1.0f / static_cast<float>(count);
		SetHitRadius(hit * inv);
		SetCooldownDuration(cooldown * inv);
		SetScanMaxRadius(scanRadius * inv);
		SetScanExpandSpeed(scanSpeed * inv);
		SetBuyPrice(price);
	}

	std::vector<std::unique_ptr<IModuleNode>> slots_;
	std::string name_;
	IconAtlas::IconBits iconBits_{};
	std::uint32_t contentRevision_{ 0 };
};