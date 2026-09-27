#pragma once
#include "IModuleNode.h"
#include "AttackNodeSteps.h"
#include "IconAtlas.h"

#include <DirectXMath.h>
#include <memory>
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
		out.push_back({ ModuleNodeUniqueStat::LifetimeRate, lifetimeRate_ });
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
		out.push_back({ ModuleNodeUniqueStat::SpeedRate, speedRate_ });
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
		out.push_back({ ModuleNodeUniqueStat::SizeRate, sizeRate_ });
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
		out.push_back({ ModuleNodeUniqueStat::DamageRate, damageRate_ });
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

	void ApplyWarehouseBonus(Attack& attack) override
	{
		attack.Stats().damage.fix += damageFix_;
	}

	void CollectUniqueStats(std::vector<ModuleNodeUniqueStatRow>& out) const override
	{
		out.push_back({ ModuleNodeUniqueStat::DamageFix, damageFix_ });
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
		out.push_back({ ModuleNodeUniqueStat::RepeatCount, repeatCount_ });
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

		// 先拷贝再重放：重放出球会 Flush，把配方和分组清掉。
		const std::size_t firstGroup = groupCount - take;
		std::vector<AttackStepRecord> window;
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
			window.insert(
				window.end(),
				ctx.recipe.begin() + static_cast<std::ptrdiff_t>(begin),
				ctx.recipe.begin() + static_cast<std::ptrdiff_t>(end));
		}

		// 复活会先打开 recordOnly。重复里的 Core / Spawn 仍要当场发射。
		// 没发射则恢复，只含属性的重放不会改掉复活后的只记账。
		// 新分组只在这次从 recordOnly 里发射之后重开，普通重放的分组不变。
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
		RedistributeChildrenEvenly(ctx.standby);
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

	void ApplyWarehouseBonus(Attack& attack) override
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