#pragma once

// 单波次内容描述。
// 时长以外的字段先填占位公式，后续加精英怪不必改调用方签名。
struct WaveSpec
{
	int index{ 1 };
	float duration{ 20.0f };
	int enemyBudget{ 8 };
	float spawnInterval{ 1.5f };
	float priceScale{ 1.0f };
	bool isElite{ false };
};

namespace WaveRules
{
	inline constexpr int kTotalWaves = 20;

	[[nodiscard]] inline constexpr float LerpUnclamped_(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	// 三段折线时长（秒）：1–5 轮 20→35，6–15 轮 35→70，16–20 轮 70→90。
	[[nodiscard]] inline constexpr float DurationSeconds_(int wave) noexcept
	{
		if (wave <= 5)
		{
			return LerpUnclamped_(20.0f, 35.0f, static_cast<float>(wave - 1) / 4.0f);
		}
		if (wave <= 15)
		{
			return LerpUnclamped_(35.0f, 70.0f, static_cast<float>(wave - 6) / 9.0f);
		}
		return LerpUnclamped_(70.0f, 90.0f, static_cast<float>(wave - 16) / 4.0f);
	}

	// 占位：每波敌人预算随波次线性上升。
	[[nodiscard]] inline constexpr int EnemyBudget_(int wave) noexcept
	{
		return 6 + wave * 2;
	}

	// 占位：生成间隔从 1.5s 线性降到 0.6s。
	[[nodiscard]] inline constexpr float SpawnInterval_(int wave) noexcept
	{
		const float t = static_cast<float>(wave - 1) / static_cast<float>(kTotalWaves - 1);
		return LerpUnclamped_(1.5f, 0.6f, t);
	}

	// 占位：商店价格系数每波 +4%。
	[[nodiscard]] inline constexpr float PriceScale_(int wave) noexcept
	{
		return 1.0f + 0.04f * static_cast<float>(wave - 1);
	}

	[[nodiscard]] inline constexpr int ClampWave_(int wave) noexcept
	{
		if (wave < 1)
		{
			return 1;
		}
		if (wave > kTotalWaves)
		{
			return kTotalWaves;
		}
		return wave;
	}

	// 按 1-based 波次号返回该波规则。越界会被夹到 [1, kTotalWaves]。
	[[nodiscard]] inline constexpr WaveSpec GetSpec(int wave) noexcept
	{
		const int n = ClampWave_(wave);
		WaveSpec spec{};
		spec.index = n;
		spec.duration = DurationSeconds_(n);
		spec.enemyBudget = EnemyBudget_(n);
		spec.spawnInterval = SpawnInterval_(n);
		spec.priceScale = PriceScale_(n);
		spec.isElite = false;
		return spec;
	}

	static_assert(kTotalWaves == 20);
	static_assert(DurationSeconds_(1) == 20.0f);
	static_assert(DurationSeconds_(5) == 35.0f);
	static_assert(DurationSeconds_(6) == 35.0f);
	static_assert(DurationSeconds_(15) == 70.0f);
	static_assert(DurationSeconds_(16) == 70.0f);
	static_assert(DurationSeconds_(20) == 90.0f);
}
