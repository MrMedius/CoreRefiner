#pragma once

#include "ModuleNodeLabel.h"

#include <array>
#include <cstddef>

namespace ModuleNodePrice
{
	inline constexpr std::array<int, ModuleNodeLabelCount()> kBuyPrice{
		0, // Core_Ball
		4, // Spawn_Ball
		4, // Attribute_LifetimeRate
		4, // Attribute_SpeedRate
		4, // Attribute_SizeRate
		4, // Attribute_DamageRate
		4, // Rule_Orbit
		4, // Rule_Return
		4, // Passive_DamageFix
		4, // Other_Child
		4, // Other_Revive
		4, // Other_Repeat
	};

	inline constexpr int kSellPriceNumerator = 1;
	inline constexpr int kSellPriceDenominator = 2;

	[[nodiscard]] constexpr int GetBuyPrice(ModuleNodeLabel label) noexcept
	{
		const std::size_t i = ToIndex(label);
		if (i >= kBuyPrice.size())
		{
			return 0;
		}
		return kBuyPrice[i];
	}

	[[nodiscard]] constexpr int GetSellPrice(ModuleNodeLabel label) noexcept
	{
		return GetBuyPrice(label) * kSellPriceNumerator / kSellPriceDenominator;
	}

	static_assert(kBuyPrice.size() == ModuleNodeLabelCount());
	static_assert(kSellPriceDenominator > kSellPriceNumerator);
	static_assert(GetBuyPrice(ModuleNodeLabel::Core_Ball) == 0);
	static_assert(GetSellPrice(ModuleNodeLabel::Spawn_Ball) < GetBuyPrice(ModuleNodeLabel::Spawn_Ball));
	static_assert(GetSellPrice(ModuleNodeLabel::Attribute_LifetimeRate) < GetBuyPrice(ModuleNodeLabel::Attribute_LifetimeRate));
	static_assert(GetSellPrice(ModuleNodeLabel::Attribute_SpeedRate) < GetBuyPrice(ModuleNodeLabel::Attribute_SpeedRate));
	static_assert(GetSellPrice(ModuleNodeLabel::Rule_Orbit) < GetBuyPrice(ModuleNodeLabel::Rule_Orbit));
	static_assert(GetSellPrice(ModuleNodeLabel::Rule_Return) < GetBuyPrice(ModuleNodeLabel::Rule_Return));
	static_assert(GetSellPrice(ModuleNodeLabel::Passive_DamageFix) < GetBuyPrice(ModuleNodeLabel::Passive_DamageFix));
	static_assert(GetSellPrice(ModuleNodeLabel::Other_Child) < GetBuyPrice(ModuleNodeLabel::Other_Child));
	static_assert(GetSellPrice(ModuleNodeLabel::Other_Revive) < GetBuyPrice(ModuleNodeLabel::Other_Revive));
	static_assert(GetSellPrice(ModuleNodeLabel::Other_Repeat) < GetBuyPrice(ModuleNodeLabel::Other_Repeat));
}
