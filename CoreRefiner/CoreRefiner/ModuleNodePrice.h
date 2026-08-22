#pragma once

#include "ModuleNodeLabel.h"

#include <array>
#include <cstddef>

namespace ModuleNodePrice
{
	inline constexpr std::array<int, ModuleNodeLabelCount()> kBuyPrice{
		4, // Spawn_Ball
		4, // Attribute_Lifetime
		4, // Attribute_SpeedRate
		4, // Rule_Orbit
		4, // Other_Child
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
	static_assert(GetSellPrice(ModuleNodeLabel::Spawn_Ball) < GetBuyPrice(ModuleNodeLabel::Spawn_Ball));
	static_assert(GetSellPrice(ModuleNodeLabel::Attribute_Lifetime) < GetBuyPrice(ModuleNodeLabel::Attribute_Lifetime));
	static_assert(GetSellPrice(ModuleNodeLabel::Attribute_SpeedRate) < GetBuyPrice(ModuleNodeLabel::Attribute_SpeedRate));
	static_assert(GetSellPrice(ModuleNodeLabel::Rule_Orbit) < GetBuyPrice(ModuleNodeLabel::Rule_Orbit));
	static_assert(GetSellPrice(ModuleNodeLabel::Other_Child) < GetBuyPrice(ModuleNodeLabel::Other_Child));
}
