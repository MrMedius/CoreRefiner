#pragma once

#include "IFieldNode.h"
#include "AttackDeployer.h"

#include <DirectXMath.h>

/**
 * @brief Field Spawn_Ball token. Scan hits Apply this the same as any other module node.
 */
class SpawnBallNode : public IFieldNode
{
public:
	explicit SpawnBallNode(
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
		DeployStep_Spawn_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Spawn_Ball"; }

protected:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

/**
 * @brief Same token as SpawnBallNode; only difference is player attack Begin when Ready.
 */
class CoreSpawnNode final : public SpawnBallNode
{
public:
	explicit CoreSpawnNode(
		DirectX::XMFLOAT2 localPos = { 0.0f, 0.0f },
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		SpawnBallNode(localPos, scale, enableCollider)
	{
		isCore_ = true;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Core/Spawn_Ball"; }
};

class ChildPitNode final : public IFieldNode
{
public:
	explicit ChildPitNode(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 1.5f;
		scanMaxRadius_ = 140.0f;
		scanExpandSpeed_ = 100.0f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		DeployStep_Other_Child::Make()->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Other_Child"; }
};

class LifetimeNode final : public IFieldNode
{
public:
	LifetimeNode(DirectX::XMFLOAT2 localPos, float durationSeconds = 2.0f) noexcept
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
		DeployStep_Attribute_Lifetime::Make(durationSeconds_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Attribute_Lifetime"; }

private:
	float durationSeconds_{ 2.0f };
};

class SpeedRateNode final : public IFieldNode
{
public:
	SpeedRateNode(DirectX::XMFLOAT2 localPos, float speedRate = 1.0f) noexcept
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
		DeployStep_Attribute_SpeedRate::Make(speedRate_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Attribute_SpeedRate"; }

private:
	float speedRate_{ 1.0f };
};

class OrbitNode final : public IFieldNode
{
public:
	OrbitNode(
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
		DeployStep_Rule_Orbit::Make(orbitRadius_, orbitAngularSpeed_, orbitPhase_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Rule_Orbit"; }

private:
	float orbitRadius_{ 2.0f };
	float orbitAngularSpeed_{ 3.5f };
	float orbitPhase_{ -1.0f };
};
