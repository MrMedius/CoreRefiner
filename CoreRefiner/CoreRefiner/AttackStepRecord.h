#pragma once

#include "ModuleNodeLabel.h"

#include <DirectXMath.h>

/**
 * @brief 一条可重放的装配 Step 快照（label + 少量参数）。
 * @note a/b/c/d/flag 含义随 label 变化；A1 只保存，不重放。
 */
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
	/** @brief Spawn_Ball：a/b/c = 缩放，flag = 碰撞开关。 */
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

	/** @brief Other_Child：无参数。 */
	inline AttackStepRecord OtherChild() noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Other_Child;
		return r;
	}

	/** @brief Attribute_LifetimeRate：a = 加算到 lifetime.rate。 */
	inline AttackStepRecord LifetimeRate(float lifetimeRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_LifetimeRate;
		r.a = lifetimeRate;
		return r;
	}

	/** @brief Attribute_SpeedRate：a = 倍率。 */
	inline AttackStepRecord SpeedRate(float speedRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_SpeedRate;
		r.a = speedRate;
		return r;
	}

	/** @brief Attribute_SizeRate：a = 加算。 */
	inline AttackStepRecord SizeRate(float sizeRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_SizeRate;
		r.a = sizeRate;
		return r;
	}

	/** @brief Attribute_DamageRate：a = 加算。 */
	inline AttackStepRecord DamageRate(float damageRate) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Attribute_DamageRate;
		r.a = damageRate;
		return r;
	}

	/** @brief Rule_Orbit：a = 半径，b = 相位。 */
	inline AttackStepRecord Orbit(float radius, float phase) noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Rule_Orbit;
		r.a = radius;
		r.b = phase;
		return r;
	}

	/** @brief Rule_Return：无参数。 */
	inline AttackStepRecord Return() noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Rule_Return;
		return r;
	}

	/** @brief Other_Revive：无参数。 */
	inline AttackStepRecord Revive() noexcept
	{
		AttackStepRecord r{};
		r.label = ModuleNodeLabel::Other_Revive;
		return r;
	}
}
