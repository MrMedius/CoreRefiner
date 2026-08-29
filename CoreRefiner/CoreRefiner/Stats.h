#pragma once

struct StatMod
{
	float base{ 0.0f };
	float add{ 0.0f };
	float mul{ 1.0f };
	float fix{ 0.0f };

	[[nodiscard]] float Final() const noexcept
	{
		return (base + add) * mul + fix;
	}

	void ResetMods() noexcept
	{
		add = 0.0f;
		mul = 1.0f;
		fix = 0.0f;
	}
};

struct CharacterStats
{
	StatMod hpMax;
	StatMod moveAccel;
	StatMod gravity{ 1.0f, 0.0f, 1.0f, 0.0f };
	StatMod moveFriction{ 0.1f, 0.0f, 1.0f, 0.0f };

	void ResetMods() noexcept
	{
		hpMax.ResetMods();
		moveAccel.ResetMods();
		gravity.ResetMods();
		moveFriction.ResetMods();
	}
};

struct AttackStats
{
	StatMod damage{ 1.0f, 0.0f, 1.0f, 0.0f };
	StatMod size{ 1.0f, 0.0f, 1.0f, 0.0f };
	StatMod speed{ 1.0f, 0.0f, 1.0f, 0.0f };

	void ResetMods() noexcept
	{
		damage.ResetMods();
		size.ResetMods();
		speed.ResetMods();
	}
};
