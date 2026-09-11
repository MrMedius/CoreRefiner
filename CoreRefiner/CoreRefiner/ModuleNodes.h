#pragma once

#include "IModuleNode.h"
#include "AttackNodeSteps.h"

#include <DirectXMath.h>

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
		cooldownDuration_ = kCooldownDuration_[0];
		scanMaxRadius_ = 100.0f;
		scanExpandSpeed_ = 100.0f;
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
		cooldownDuration_ = kCooldownDuration_[level_.Index()];
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
		cooldownDuration_ = kCooldownDuration_[0];
		scanMaxRadius_ = 80.0f;
		scanExpandSpeed_ = 80.0f;
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
		cooldownDuration_ = kCooldownDuration_[level_.Index()];
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
		cooldownDuration_ = 2.0f;
		scanMaxRadius_ = 50.0f;
		scanExpandSpeed_ = 50.0f;
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
		cooldownDuration_ = 2.0f;
		scanMaxRadius_ = 50.0f;
		scanExpandSpeed_ = 50.0f;
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
		cooldownDuration_ = 2.0f;
		scanMaxRadius_ = 50.0f;
		scanExpandSpeed_ = 50.0f;
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
		cooldownDuration_ = 2.0f;
		scanMaxRadius_ = 50.0f;
		scanExpandSpeed_ = 50.0f;
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
		hitRadius_ = 20.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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

private:
	float orbitRadius_{ 2.0f };
	float orbitPhase_{ -1.0f };
};

class ModuleNode_Rule_Return final : public IModuleNode
{
public:
	explicit ModuleNode_Rule_Return(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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
		hitRadius_ = kHitRadius_[level_.Index()];
	}

private:
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 20.0f, 16.0f, 12.0f };
};

class ModuleNode_Passive_DamageFix final : public IModuleNode
{
public:
	explicit ModuleNode_Passive_DamageFix(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		damageFix_ = kDamageFix_[0];
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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
		hitRadius_ = kHitRadius_[level_.Index()];
		damageFix_ = kDamageFix_[level_.Index()];
	}

private:
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 16.0f, 16.0f, 16.0f };
	static constexpr float kDamageFix_[ModuleNodeLevel::kCount] = { 1.0f, 1.5f, 2.0f };
	float damageFix_{ 1.0f };
};

class ModuleNode_Other_Child final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Child(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 8.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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
};

class ModuleNode_Other_Revive final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Revive(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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
		hitRadius_ = kHitRadius_[level_.Index()];
	}

private:
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 16.0f, 14.0f, 12.0f };
};

class ModuleNode_Other_Repeat final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Repeat(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = kHitRadius_[0];
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Other_Repeat::Make()->Apply(ctx);
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
		hitRadius_ = kHitRadius_[level_.Index()];
	}

private:
	static constexpr float kHitRadius_[ModuleNodeLevel::kCount] = { 16.0f, 14.0f, 12.0f };
};