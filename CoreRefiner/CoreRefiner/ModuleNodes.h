#pragma once

#include "IModuleNode.h"
#include "AttackNodeSteps.h"
#include "IconAtlas.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

class ModuleNode_Spawn_Ball_Core final : public IModuleNode
{
public:
	explicit ModuleNode_Spawn_Ball_Core(
		DirectX::XMFLOAT2 localPos = { 0.0f, 0.0f },
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
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
		AttackNodeStep_Spawn_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Core_Ball;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Core;
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

class ModuleNode_Spawn_Ball final : public IModuleNode
{
public:
	explicit ModuleNode_Spawn_Ball(
		DirectX::XMFLOAT2 localPos,
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Spawn_Ball;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Spawn;
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

class ModuleNode_Attribute_LifetimeRate final : public IModuleNode
{
public:
	explicit ModuleNode_Attribute_LifetimeRate(DirectX::XMFLOAT2 localPos) noexcept
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_LifetimeRate;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Attribute;
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SpeedRate;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Attribute;
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_SizeRate;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Attribute;
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_DamageRate;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Attribute;
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

class ModuleNode_Rule_Orbit final : public IModuleNode
{
public:
	ModuleNode_Rule_Orbit(
		DirectX::XMFLOAT2 localPos,
		float radius = 2.0f,
		float phase = -1.0f) noexcept
		:
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Orbit;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Rule;
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Return;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Rule;
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

class ModuleNode_Passive_DamageFix final : public IModuleNode
{
public:
	explicit ModuleNode_Passive_DamageFix(DirectX::XMFLOAT2 localPos) noexcept
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Passive_DamageFix;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Passive;
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

class ModuleNode_Other_Child final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Child(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 20.0f;
		SetCooldownDuration(2.0f);
		SetScanMaxRadius(80.0f);
		SetScanExpandSpeed(kscanExpandSpeed_[0]);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Other_Child::Make()->Apply(ctx);
	}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Child;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Other;
	}

protected:
	void ApplyLevelStats_() override
	{
		SetScanExpandSpeed(kscanExpandSpeed_[level_.Index()]);
	}

private:
	static constexpr float kscanExpandSpeed_[ModuleNodeLevel::kCount] = { 50.0f, 75.0f, 100.0f };
};

class ModuleNode_Other_Revive final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Revive(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 15.0f;
		SetCooldownDuration(kCooldownDuration_[0]);
		SetScanMaxRadius(100.0f);
		SetScanExpandSpeed(100.0f);
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Other_Revive::Make()->Apply(ctx);
	}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Revive;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Other;
	}

protected:
	void ApplyLevelStats_() override
	{
		SetCooldownDuration(kCooldownDuration_[level_.Index()]);
	}

private:
	static constexpr float kCooldownDuration_[ModuleNodeLevel::kCount] = { 5.0f, 4.0f, 3.0f };
};

class ModuleNode_Other_Repeat final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Repeat(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 15.0f;
		SetCooldownDuration(3.0f);
		SetScanMaxRadius(50.0f);
		SetScanExpandSpeed(50.0f);
		repeatCount_ = kRepeatCount_[0];
	}

	void ApplyTo(DeployContext& ctx) override
	{
		if (ctx.recipe.empty())
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
		if (take > ctx.recipe.size())
		{
			take = ctx.recipe.size();
		}

		const std::vector<AttackStepRecord> window(
			ctx.recipe.end() - static_cast<std::ptrdiff_t>(take),
			ctx.recipe.end());
		for (const AttackStepRecord& rec : window)
		{
			ApplyAttackStepRecord(ctx, rec);
		}
		RedistributeChildrenEvenly(ctx.standby);
	}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Other_Repeat;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Other;
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

class ModuleNode_Fusion final : public IModuleNode
{
public:
	ModuleNode_Fusion(std::unique_ptr<IModuleNode> primary, std::unique_ptr<IModuleNode> material, DirectX::XMFLOAT2 localPos) noexcept
		:
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

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Fusion;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Fusion;
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

	void InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 zoneOrigin) override
	{
		zoneOrigin_ = zoneOrigin;

		icon_ = std::make_unique<Canvas2D>(gfx, kVisualSize, kVisualSize);
		icon_->Clear(Colors::None);
		BlitHalves_(*icon_, false);
		icon_->NotifyPixelsChanged();
		icon_->LinkTechniques(rg);

		mask_ = std::make_unique<Canvas2DSpriteUV>(gfx, kVisualSize, kVisualSize);
		mask_->Clear(Colors::None);
		BlitHalves_(*mask_, true);
		mask_->NotifyPixelsChanged();
		mask_->LinkTechniques(rg);
		mask_->SetUVOffset(0.0f, 0.0f);
		mask_->SetUVScale(1.0f, 0.0f);

		visualReady_ = true;

		SyncMaskUV_();
		ApplyVisualTransform_();
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