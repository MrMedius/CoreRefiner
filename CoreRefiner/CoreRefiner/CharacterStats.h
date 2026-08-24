#pragma once

struct StatMod
{
	float base{ 0.0f };
	float add{ 0.0f };
	float mul{ 1.0f };

	[[nodiscard]] float Final() const noexcept
	{
		return (base + add) * mul;
	}

	void ResetMods() noexcept
	{
		add = 0.0f;
		mul = 1.0f;
	}
};

struct CharacterStats
{
	StatMod hpMax;
	StatMod moveAccel;

	void ResetMods() noexcept
	{
		hpMax.ResetMods();
		moveAccel.ResetMods();
	}
};
