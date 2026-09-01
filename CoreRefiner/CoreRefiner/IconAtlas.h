#pragma once
#include "ModuleNodeLabel.h"
#include "Canvas.h"
#include "Colors.h"

#include <array>
#include <cassert>
#include <cstdint>

namespace IconAtlas
{
	using IconBits = std::array<std::uint16_t, 16>;

	inline void BlitIcon(Canvas& canvas, const IconBits& bits, Color color)
	{
		const unsigned w = canvas.GetCanvasWidth();
		const unsigned h = canvas.GetCanvasHeight();
		const unsigned rows = (h < 16u) ? h : 16u;
		const unsigned cols = (w < 16u) ? w : 16u;

		for (unsigned y = 0u; y < rows; ++y)
		{
			const std::uint16_t row = bits[y];
			for (unsigned x = 0u; x < cols; ++x)
			{
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) != 0u)
				{
					canvas.PutPixel(x, y, color);
				}
			}
		}
	}
}

namespace NodeIconAtlas
{
	namespace detail
	{
		inline constexpr IconAtlas::IconBits kRoundFrame{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000000000100,
			0b0100000000000010,
			0b0100000000000010,
			0b1000000000000001,
			0b1000000000000001,
			0b1000000000000001,
			0b1000000000000001,
			0b0100000000000010,
			0b0100000000000010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};
		inline constexpr IconAtlas::IconBits kSpawnBall{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000001000100,
			0b0100000010010010,
			0b0100000100100010,
			0b1000000001001001,
			0b1000011100010001,
			0b1000100010100001,
			0b1000100010000001,
			0b0100100010000010,
			0b0100011100000010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kAttributeLifetime{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010011111100100,
			0b0100100000010010,
			0b0100010000100010,
			0b1000001001000001,
			0b1000000110000001,
			0b1000000110000001,
			0b1000001001000001,
			0b0100010000100010,
			0b0100100000010010,
			0b0010011111100100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kAttributeSpeedRate{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000010001000,
			0b0010010001000100,
			0b0100001000100010,
			0b0101000100010010,
			0b1000100010001001,
			0b1000010001000101,
			0b1000010001000101,
			0b1000100010001001,
			0b0101000100010010,
			0b0100001000100010,
			0b0010010001000100,
			0b0001000010001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kAttributeSizeRate{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000000000100,
			0b0100111001110010,
			0b0100110000110010,
			0b1000101001010001,
			0b1000000100000001,
			0b1000000010000001,
			0b1000101001010001,
			0b0100110000110010,
			0b0100111001110010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kAttributeDamageRate{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000000000100,
			0b0100000111110010,
			0b0100000001110010,
			0b1000000011110001,
			0b1000110111010001,
			0b1000011110000001,
			0b1000001100000001,
			0b0100010110000010,
			0b0100100010000010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kRuleOrbit{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000000000100,
			0b0100001111000010,
			0b0100010000100010,
			0b1001100000011001,
			0b1010100000010101,
			0b1010100000010101,
			0b1001101101011001,
			0b0100010000100010,
			0b0100001111000010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kRuleReturn{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000000000100,
			0b0100000000010010,
			0b0100000100100010,
			0b1000000101000001,
			0b1000000110000001,
			0b1000010111100001,
			0b1000000000000001,
			0b0101111100000010,
			0b0100111000000010,
			0b0010010000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};

		inline constexpr IconAtlas::IconBits kOtherChild{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010110000000100,
			0b0101001000000010,
			0b0101001111000010,
			0b1000110000100001,
			0b1000010000100001,
			0b1000010000100001,
			0b1000010000110001,
			0b0100001111001010,
			0b0100000001001010,
			0b0010000000110100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};
	}

	inline constexpr std::array<IconAtlas::IconBits, ModuleNodeLabelCount()> kNodeIcons{
		detail::kSpawnBall,           // Core_Ball
		detail::kSpawnBall,           // Spawn_Ball
		detail::kAttributeLifetime,   // Attribute_Lifetime
		detail::kAttributeSpeedRate,  // Attribute_SpeedRate
		detail::kAttributeSizeRate,   // Attribute_SizeRate
		detail::kAttributeDamageRate, // Attribute_DamageRate
		detail::kRuleOrbit,           // Rule_Orbit
		detail::kRuleReturn,          // Rule_Return
		detail::kRoundFrame,          // Passive_DamageFix
		detail::kOtherChild,          // Other_Child
		detail::kRoundFrame,          // Other_Revive
	};

	static_assert(kNodeIcons.size() == ModuleNodeLabelCount());

	[[nodiscard]] inline const IconAtlas::IconBits& Get(ModuleNodeLabel id) noexcept
	{
		const std::size_t i = ToIndex(id);
		assert(i < ModuleNodeLabelCount());
		if (i >= ModuleNodeLabelCount())
		{
			return kNodeIcons[0];
		}
		return kNodeIcons[i];
	}
}

enum class UiIconId : unsigned char
{
	Currency,
	Refresh,
	LockOpen,
	LockClosed,
	Count
};

[[nodiscard]] inline constexpr std::size_t UiIconIdCount() noexcept
{
	return static_cast<std::size_t>(UiIconId::Count);
}

namespace UiIconAtlas
{
	namespace detail
	{
		inline constexpr IconAtlas::IconBits kCurrency{
			0b0000000000000000,
			0b0000001001000000,
			0b0000001111000000,
			0b0000111001110000,
			0b0001001001001000,
			0b0001001001001000,
			0b0000111001000000,
			0b0000001111000000,
			0b0000001001110000,
			0b0001001001001000,
			0b0001001001001000,
			0b0001001001001000,
			0b0000111001110000,
			0b0000001111000000,
			0b0000001001000000,
			0b0000000000000000,
		};

		inline constexpr IconAtlas::IconBits kRefresh{
			0b0000000000000000,
			0b0000000100000000,
			0b0000000110000000,
			0b0000011111000000,
			0b0000100110000000,
			0b0001000100000100,
			0b0010000000000100,
			0b0010000000000100,
			0b0010000000000100,
			0b0010000000000100,
			0b0010000010001000,
			0b0000000110010000,
			0b0000001111100000,
			0b0000000110000000,
			0b0000000010000000,
			0b0000000000000000,
		};

		inline constexpr IconAtlas::IconBits kLockOpen{
			0b0000001111100000,
			0b0000110000110000,
			0b0000100000010000,
			0b0000100000010000,
			0b0000100000010000,
			0b0000100000000000,
			0b0011111111111100,
			0b0011000000001100,
			0b0011000110001100,
			0b0011000110001100,
			0b0011000000001100,
			0b0011000110001100,
			0b0011000000001100,
			0b0011111111111100,
			0b0000000000000000,
			0b0000000000000000,
		};

		inline constexpr IconAtlas::IconBits kLockClosed{
			0b0000000000000000,
			0b0000011111100000,
			0b0000110000110000,
			0b0000100000010000,
			0b0000100000010000,
			0b0000100000010000,
			0b0011111111111100,
			0b0011000000001100,
			0b0011000110001100,
			0b0011000110001100,
			0b0011000000001100,
			0b0011000110001100,
			0b0011000000001100,
			0b0011111111111100,
			0b0000000000000000,
			0b0000000000000000,
		};
	}

	inline constexpr std::array<IconAtlas::IconBits, UiIconIdCount()> kUiIcons{
		detail::kCurrency,
		detail::kRefresh,
		detail::kLockOpen,
		detail::kLockClosed,
	};

	[[nodiscard]] inline const IconAtlas::IconBits& Get(UiIconId id) noexcept
	{
		const std::size_t i = static_cast<std::size_t>(id);
		assert(i < UiIconIdCount());
		if (i >= UiIconIdCount())
		{
			return kUiIcons[0];
		}
		return kUiIcons[i];
	}
}
