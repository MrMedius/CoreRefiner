#pragma once

#include "IModuleNode.h"
#include "AttackNodeSteps.h"

#include <DirectXMath.h>

class ModuleNode_Spawn_Ball : public IModuleNode
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
		hitRadius_ = 24.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

class ModuleNode_Spawn_Ball_Core final : public ModuleNode_Spawn_Ball
{
public:
	explicit ModuleNode_Spawn_Ball_Core(
		DirectX::XMFLOAT2 localPos = { 0.0f, 0.0f },
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		ModuleNode_Spawn_Ball(localPos, scale, enableCollider)
	{
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Core_Ball;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Core;
	}
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

class ModuleNode_Attribute_Lifetime final : public IModuleNode
{
public:
	ModuleNode_Attribute_Lifetime(DirectX::XMFLOAT2 localPos, float durationSeconds = 2.0f) noexcept
		:
		durationSeconds_(durationSeconds)
	{
		localPos_ = localPos;
		hitRadius_ = 16.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_Lifetime::Make(durationSeconds_)->Apply(ctx);
	}

	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Attribute_Lifetime;
	}

	[[nodiscard]] ModuleNodeKind GetKind() const noexcept override
	{
		return ModuleNodeKind::Attribute;
	}

private:
	float durationSeconds_{ 2.0f };
};

class ModuleNode_Attribute_SpeedRate final : public IModuleNode
{
public:
	ModuleNode_Attribute_SpeedRate(DirectX::XMFLOAT2 localPos, float speedRate = 1.0f) noexcept
		:
		speedRate_(speedRate)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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

private:
	float speedRate_{ 1.0f };
};

class ModuleNode_Attribute_SizeRate final : public IModuleNode
{
public:
	ModuleNode_Attribute_SizeRate(DirectX::XMFLOAT2 localPos, float sizeRate = 0.5f) noexcept
		:
		sizeRate_(sizeRate)
	{
		localPos_ = localPos;
		hitRadius_ = 12.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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

private:
	float sizeRate_{ 0.5f };
};

class ModuleNode_Attribute_DamageRate final : public IModuleNode
{
public:
	ModuleNode_Attribute_DamageRate(DirectX::XMFLOAT2 localPos, float damageRate = 0.5f) noexcept
		:
		damageRate_(damageRate)
	{
		localPos_ = localPos;
		hitRadius_ = 24.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
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

private:
	float damageRate_{ 0.5f };
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
		hitRadius_ = 20.0f;
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
};

class ModuleNode_Passive_DamageFix final : public IModuleNode
{
public:
	explicit ModuleNode_Passive_DamageFix(DirectX::XMFLOAT2 localPos, float damageFix = 1.0f) noexcept
		:
		damageFix_(damageFix)
	{
		localPos_ = localPos;
		hitRadius_ = 16.0f;
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

private:
	float damageFix_{ 1.0f };
};

class ModuleNode_Other_Revive final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Revive(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 16.0f;
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
};

class ModuleNode_Other_Repeat final : public IModuleNode
{
public:
	explicit ModuleNode_Other_Repeat(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 16.0f;
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
};