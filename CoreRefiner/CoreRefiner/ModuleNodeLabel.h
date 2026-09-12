#pragma once
#include <cstddef>

enum class ModuleNodeKind : unsigned char
{
	Core,
	Spawn,
	Attribute,
	Rule,
	Passive,
	Other,
	Fusion,
	Count
};

enum class ModuleNodeLabel : unsigned char
{
	Core_Ball,

	Spawn_Ball,

	Attribute_LifetimeRate,
	Attribute_SpeedRate,
	Attribute_SizeRate,
	Attribute_DamageRate,

	Rule_Orbit,
	Rule_Return,

	Passive_DamageFix,

	Other_Child,
	Other_Revive,
	Other_Repeat,

	Fusion,

	Count
};

[[nodiscard]] inline constexpr std::size_t ModuleNodeLabelCount() noexcept
{
	return static_cast<std::size_t>(ModuleNodeLabel::Count);
}

[[nodiscard]] inline constexpr std::size_t ModuleNodeKindCount() noexcept
{
	return static_cast<std::size_t>(ModuleNodeKind::Count);
}

[[nodiscard]] inline constexpr std::size_t ToIndex(ModuleNodeLabel id) noexcept
{
	return static_cast<std::size_t>(id);
}

[[nodiscard]] inline constexpr std::size_t ToIndex(ModuleNodeKind id) noexcept
{
	return static_cast<std::size_t>(id);
}
