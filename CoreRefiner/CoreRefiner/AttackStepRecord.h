#pragma once

#include "ModuleNodeLabel.h"

#include <DirectXMath.h>

// 一条可重放的装配 Step 快照（label + 少量参数）。
// a/b/c/d/flag 含义随 label 变化；A1 只保存，不重放。
struct AttackStepRecord
{
	ModuleNodeLabel label{ ModuleNodeLabel::Count };
	float a{ 0.0f };
	float b{ 0.0f };
	float c{ 0.0f };
	float d{ 0.0f };
	bool flag{ false };
};

namespace AttackStepRecordMake
{
	// ————————————————————————————————————————————————————
	// Kind —— Core
	// ————————————————————————————————————————————————————
	// Core_Ball：a/b/c = 缩放，flag = 碰撞开关。
	inline AttackStepRecord CoreBall(DirectX::XMFLOAT3 scale, bool enableCollider) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Core_Ball;
		r.a = scale.x;
		r.b = scale.y;
		r.c = scale.z;
		r.flag = enableCollider;
		return r;
	}

	// ————————————————————————————————————————————————————
	// Kind —— Spawn
	// ————————————————————————————————————————————————————
	// Spawn_Ball：a/b/c = 缩放，flag = 碰撞开关。
	inline AttackStepRecord SpawnBall(DirectX::XMFLOAT3 scale, bool enableCollider) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Spawn_Ball;
		r.a = scale.x;
		r.b = scale.y;
		r.c = scale.z;
		r.flag = enableCollider;
		return r;
	}

	// ————————————————————————————————————————————————————
	// Kind —— Attribute
	// ————————————————————————————————————————————————————
	// Attribute_LifetimeRate：a = 加算到 lifetime.rate。
	inline AttackStepRecord LifetimeRate(float lifetimeRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_LifetimeRate;
		r.a = lifetimeRate;
		return r;
	}

	// Attribute_SpeedRate：a = 倍率。
	inline AttackStepRecord SpeedRate(float speedRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_SpeedRate;
		r.a = speedRate;
		return r;
	}

	// Attribute_SizeRate：a = 加算。
	inline AttackStepRecord SizeRate(float sizeRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_SizeRate;
		r.a = sizeRate;
		return r;
	}

	// Attribute_DamageRate：a = 加算。
	inline AttackStepRecord DamageRate(float damageRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_DamageRate;
		r.a = damageRate;
		return r;
	}

	// ————————————————————————————————————————————————————
	// Kind —— Rule
	// ————————————————————————————————————————————————————
	// Rule_Orbit：a = 半径，b = 相位。
	inline AttackStepRecord Orbit(float radius, float phase) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Rule_Orbit;
		r.a = radius;
		r.b = phase;
		return r;
	}

	// Rule_Return：无参数。
	inline AttackStepRecord Return() noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Rule_Return;
		return r;
	}

	// Rule_Child：无参数。
	inline AttackStepRecord Child() noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Rule_Child;
		return r;
	}

	// Rule_Revive：无参数。
	inline AttackStepRecord Revive() noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Rule_Revive;
		return r;
	}

	// ————————————————————————————————————————————————————
	// Kind —— Passive
	// ————————————————————————————————————————————————————
	
	// ————————————————————————————————————————————————————
	// Kind —— Other
	// ————————————————————————————————————————————————————
	
	// ————————————————————————————————————————————————————
	// Kind —— Fusion
	// ————————————————————————————————————————————————————
}