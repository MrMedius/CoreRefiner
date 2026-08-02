#pragma once

#include "FieldModuleNode.h"
#include "AttackDeployer.h"

#include <DirectXMath.h>

class CoreSpawnNode final : public FieldModuleNode
{
public:
	explicit CoreSpawnNode(
		DirectX::XMFLOAT2 localPos = { 0.0f, 0.0f },
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{
		isCore_ = true;
		localPos_ = localPos;
		hitRadius_ = 22.0f;
		cooldownDuration_ = 0.5f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		DeployStep_Spawn_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Core/Spawn_Ball"; }

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

class SpawnBallNode final : public FieldModuleNode
{
public:
	explicit SpawnBallNode(
		DirectX::XMFLOAT2 localPos,
		DirectX::XMFLOAT3 scale = { 0.35f, 0.35f, 0.35f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{
		localPos_ = localPos;
		hitRadius_ = 16.0f;
		cooldownDuration_ = 0.75f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		DeployStep_Spawn_Ball::Make(scale_, enableCollider_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Spawn_Ball"; }

private:
	DirectX::XMFLOAT3 scale_{ 0.35f, 0.35f, 0.35f };
	bool enableCollider_{ true };
};

class ChildPitNode final : public FieldModuleNode
{
public:
	explicit ChildPitNode(DirectX::XMFLOAT2 localPos) noexcept
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 0.75f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		DeployStep_Other_Child::Make()->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Other_Child"; }
};

class LifetimeNode final : public FieldModuleNode
{
public:
	LifetimeNode(DirectX::XMFLOAT2 localPos, float durationSeconds = 2.0f) noexcept
		:
		durationSeconds_(durationSeconds)
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 0.75f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		DeployStep_Attribute_Lifetime::Make(durationSeconds_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Attribute_Lifetime"; }

private:
	float durationSeconds_{ 2.0f };
};

class SpeedRateNode final : public FieldModuleNode
{
public:
	SpeedRateNode(DirectX::XMFLOAT2 localPos, float speedRate = 1.0f) noexcept
		:
		speedRate_(speedRate)
	{
		localPos_ = localPos;
		hitRadius_ = 14.0f;
		cooldownDuration_ = 0.75f;
	}

	void ApplyTo(DeployContext& ctx) override
	{
		DeployStep_Attribute_SpeedRate::Make(speedRate_)->Apply(ctx);
	}

	[[nodiscard]] const char* GetLabel() const noexcept override { return "Attribute_SpeedRate"; }

private:
	float speedRate_{ 1.0f };
};

class OrbitNode final : public FieldModuleNode
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
		cooldownDuration_ = 0.75f;
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
