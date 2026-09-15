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
	// ————————————————————————————————————————————————————
	// Kind —— Core
	// ————————————————————————————————————————————————————
	Core_Ball,
	// ————————————————————————————————————————————————————
	// Kind —— Spawn
	// ————————————————————————————————————————————————————
	Spawn_Ball,
	// ————————————————————————————————————————————————————
	// Kind —— Attribute
	// ————————————————————————————————————————————————————
	Attribute_LifetimeRate,
	Attribute_SpeedRate,
	Attribute_SizeRate,
	Attribute_DamageRate,
	// ————————————————————————————————————————————————————
	// Kind —— Rule
	// ————————————————————————————————————————————————————
	Rule_Orbit,
	Rule_Return,
	Rule_Child,
	Rule_Revive,
	// ————————————————————————————————————————————————————
	// Kind —— Passive
	// ————————————————————————————————————————————————————
	Passive_DamageFix,
	// ————————————————————————————————————————————————————
	// Kind —— Other
	// ————————————————————————————————————————————————————
	Other_Repeat,
	// ————————————————————————————————————————————————————
	// Kind —— Fusion
	// ————————————————————————————————————————————————————
	Fusion,
	// ————————————————————————————————————————————————————
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

// 每个 Kind 一张 Label 子表
// ————————————————————————————————————————————————————
// Kind —— Core
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kCoreLabels[] = {
	ModuleNodeLabel::Core_Ball,
};
// ————————————————————————————————————————————————————
// Kind —— Spawn
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kSpawnLabels[] = {
	ModuleNodeLabel::Spawn_Ball,
};
// ————————————————————————————————————————————————————
// Kind —— Attribute
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kAttributeLabels[] = {
	ModuleNodeLabel::Attribute_LifetimeRate,
	ModuleNodeLabel::Attribute_SpeedRate,
	ModuleNodeLabel::Attribute_SizeRate,
	ModuleNodeLabel::Attribute_DamageRate,
};
// ————————————————————————————————————————————————————
// Kind —— Rule
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kRuleLabels[] = {
	ModuleNodeLabel::Rule_Orbit,
	ModuleNodeLabel::Rule_Return,
	ModuleNodeLabel::Rule_Child,
	ModuleNodeLabel::Rule_Revive,
};
// ————————————————————————————————————————————————————
// Kind —— Passive
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kPassiveLabels[] = {
	ModuleNodeLabel::Passive_DamageFix,
};
// ————————————————————————————————————————————————————
// Kind —— Other
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kOtherLabels[] = {
	ModuleNodeLabel::Other_Repeat,
};
// ————————————————————————————————————————————————————
// Kind —— Fusion
// ————————————————————————————————————————————————————
inline constexpr ModuleNodeLabel kFusionLabels[] = {
	ModuleNodeLabel::Fusion,
};

struct ModuleNodeLabelTable
{
	const ModuleNodeLabel* data;
	std::size_t count;
};

template<std::size_t N>
[[nodiscard]] inline constexpr bool LabelTableContains(
	const ModuleNodeLabel (&table)[N],
	ModuleNodeLabel label) noexcept
{
	for (std::size_t i = 0; i < N; ++i)
	{
		if (table[i] == label)
		{
			return true;
		}
	}
	return false;
}

template<std::size_t N>
[[nodiscard]] inline constexpr ModuleNodeLabelTable MakeLabelTable(
	const ModuleNodeLabel (&table)[N]) noexcept
{
	return { table, N };
}

[[nodiscard]] inline constexpr ModuleNodeLabelTable LabelsOf(ModuleNodeKind kind) noexcept
{
	switch (kind)
	{
	case ModuleNodeKind::Core:
		return MakeLabelTable(kCoreLabels);
	case ModuleNodeKind::Spawn:
		return MakeLabelTable(kSpawnLabels);
	case ModuleNodeKind::Attribute:
		return MakeLabelTable(kAttributeLabels);
	case ModuleNodeKind::Rule:
		return MakeLabelTable(kRuleLabels);
	case ModuleNodeKind::Passive:
		return MakeLabelTable(kPassiveLabels);
	case ModuleNodeKind::Other:
		return MakeLabelTable(kOtherLabels);
	case ModuleNodeKind::Fusion:
		return MakeLabelTable(kFusionLabels);
	default:
		return { nullptr, 0 };
	}
}

// 检测各Kind表并起来 = Label 全集，不漏不叠
[[nodiscard]] inline constexpr bool KindTablesPartitionLabels() noexcept
{
	std::size_t total = 0;
	for (std::size_t k = 0; k < ModuleNodeKindCount(); ++k)
	{
		const ModuleNodeLabelTable table = LabelsOf(static_cast<ModuleNodeKind>(k));
		if (table.count == 0u || table.data == nullptr)
		{
			return false;
		}
		total += table.count;
	}
	if (total != ModuleNodeLabelCount())
	{
		return false;
	}
	for (std::size_t i = 0; i < ModuleNodeLabelCount(); ++i)
	{
		const auto label = static_cast<ModuleNodeLabel>(i);
		std::size_t hits = 0;
		for (std::size_t k = 0; k < ModuleNodeKindCount(); ++k)
		{
			const ModuleNodeLabelTable table = LabelsOf(static_cast<ModuleNodeKind>(k));
			for (std::size_t n = 0; n < table.count; ++n)
			{
				if (table.data[n] == label)
				{
					++hits;
				}
			}
		}
		if (hits != 1u)
		{
			return false;
		}
	}
	return true;
}
static_assert(KindTablesPartitionLabels());


// 进化产物子集，项仍留在对应 Kind 表里，不从 Kind 表抠走。
inline constexpr ModuleNodeLabel kEvolveLabels[] = {
	ModuleNodeLabel::Rule_Revive,
};

[[nodiscard]] inline constexpr bool LabelInAnyKindTable(ModuleNodeLabel label) noexcept
{
	for (std::size_t k = 0; k < ModuleNodeKindCount(); ++k)
	{
		const ModuleNodeLabelTable table = LabelsOf(static_cast<ModuleNodeKind>(k));
		for (std::size_t n = 0; n < table.count; ++n)
		{
			if (table.data[n] == label)
			{
				return true;
			}
		}
	}
	return false;
}

[[nodiscard]] inline constexpr bool EvolveLabelsSubsetOfKindTables() noexcept
{
	for (std::size_t i = 0; i < sizeof(kEvolveLabels) / sizeof(kEvolveLabels[0]); ++i)
	{
		const ModuleNodeLabel label = kEvolveLabels[i];
		if (label >= ModuleNodeLabel::Count || !LabelInAnyKindTable(label))
		{
			return false;
		}
		for (std::size_t j = i + 1u; j < sizeof(kEvolveLabels) / sizeof(kEvolveLabels[0]); ++j)
		{
			if (kEvolveLabels[j] == label)
			{
				return false;
			}
		}
	}
	return true;
}
static_assert(EvolveLabelsSubsetOfKindTables());