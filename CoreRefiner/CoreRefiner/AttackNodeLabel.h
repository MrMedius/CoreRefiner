#pragma once

#include <cstddef>

enum class AttackNodeLabel : unsigned char
{
	Spawn_Ball,

	Attribute_Lifetime,

	Attribute_SpeedRate,

	Rule_Orbit,

	Other_Child,

	Count
};

[[nodiscard]] inline constexpr std::size_t AttackNodeLabelCount() noexcept
{
	return static_cast<std::size_t>(AttackNodeLabel::Count);
}

[[nodiscard]] inline constexpr std::size_t ToIndex(AttackNodeLabel id) noexcept
{
	return static_cast<std::size_t>(id);
}
