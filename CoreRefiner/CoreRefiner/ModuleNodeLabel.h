#pragma once

#include <cstddef>

enum class ModuleNodeLabel : unsigned char
{
	Spawn_Ball,

	Attribute_Lifetime,

	Attribute_SpeedRate,

	Rule_Orbit,

	Other_Child,

	Count
};

[[nodiscard]] inline constexpr std::size_t ModuleNodeLabelCount() noexcept
{
	return static_cast<std::size_t>(ModuleNodeLabel::Count);
}

[[nodiscard]] inline constexpr std::size_t ToIndex(ModuleNodeLabel id) noexcept
{
	return static_cast<std::size_t>(id);
}
