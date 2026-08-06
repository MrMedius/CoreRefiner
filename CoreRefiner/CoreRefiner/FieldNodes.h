#pragma once

#include "IFieldNode.h"
#include "AttackDeployer.h"

#include <DirectXMath.h>

class FieldNode_Spawn_Ball : public IFieldNode
{
public:
	explicit FieldNode_Spawn_Ball(
		DirectX::XMFLOAT2 localPos,
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{
		localPos_ = localPos;
		hitRadius_ = 16.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Spawn_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Spawn_Ball;
	}

protected:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

class FieldNode_Spawn_Ball_Core final : public FieldNode_Spawn_Ball
{
public:
	explicit FieldNode_Spawn_Ball_Core(
		DirectX::XMFLOAT2 localPos = { 0.0f, 0.0f },
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		FieldNode_Spawn_Ball(localPos, scale, enableCollider)
	{
		isCore_ = true;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}
};

class FieldNode_Other_Child final : public IFieldNode
{
public:
	explicit FieldNode_Other_Child(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Other_Child::Make()->Apply(ctx);
	}

	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Other_Child;
	}
};

class FieldNode_Attribute_Lifetime final : public IFieldNode
{
public:
	FieldNode_Attribute_Lifetime(DirectX::XMFLOAT2 localPos, float durationSeconds = 2.0f) noexcept
		:
		durationSeconds_(durationSeconds)
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_Lifetime::Make(durationSeconds_)->Apply(ctx);
	}

	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Attribute_Lifetime;
	}

private:
	float durationSeconds_{ 2.0f };
};

class FieldNode_Attribute_SpeedRate final : public IFieldNode
{
public:
	FieldNode_Attribute_SpeedRate(DirectX::XMFLOAT2 localPos, float speedRate = 1.0f) noexcept
		:
		speedRate_(speedRate)
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Attribute_SpeedRate::Make(speedRate_)->Apply(ctx);
	}

	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Attribute_SpeedRate;
	}

private:
	float speedRate_{ 1.0f };
};

class FieldNode_Rule_Orbit final : public IFieldNode
{
public:
	FieldNode_Rule_Orbit(
		DirectX::XMFLOAT2 localPos,
		float radius = 2.0f,
		float angularSpeed = 3.5f,
		float phase = -1.0f) noexcept
		:
		orbitRadius_(radius),
		orbitAngularSpeed_(angularSpeed),
		orbitPhase_(phase)
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		AttackNodeStep_Rule_Orbit::Make(orbitRadius_, orbitAngularSpeed_, orbitPhase_)->Apply(ctx);
	}

	[[nodiscard]] AttackNodeLabel GetAttackNodeLabel() const noexcept override
	{
		return AttackNodeLabel::Rule_Orbit;
	}

private:
	float orbitRadius_{ 2.0f };
	float orbitAngularSpeed_{ 3.5f };
	float orbitPhase_{ -1.0f };
};
