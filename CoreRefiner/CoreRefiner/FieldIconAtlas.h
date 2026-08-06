#pragma once
#include "AttackNodeLabel.h"
#include "Canvas.h"
#include "Colors.h"

#include <array>
#include <cassert>
#include <cstdint>


// 16x16 1-bit icon: each uint16_t is one row; bit0 = leftmost column.
using FieldIconBits = std::array<std::uint16_t, 16>;

namespace FieldIconAtlas
{
	namespace detail
	{
		inline constexpr FieldIconBits kSpawnBall{
			0b0000000000000000,
			0b0000000110000000,
			0b0000001111000000,
			0b0000011111100000,
			0b0000111111110000,
			0b0001111111111000,
			0b0011111111111100,
			0b0011111111111100,
			0b0011111111111100,
			0b0011111111111100,
			0b0001111111111000,
			0b0000111111110000,
			0b0000011111100000,
			0b0000001111000000,
			0b0000000110000000,
			0b0000000000000000,
		};

		inline constexpr FieldIconBits kAttributeLifetime{
			0b0000000000000000,
			0b0011111111111100,
			0b0011111111111100,
			0b0001111111111000,
			0b0000111111110000,
			0b0000011111100000,
			0b0000001111000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000001111000000,
			0b0000011111100000,
			0b0000111111110000,
			0b0001111111111000,
			0b0011111111111100,
			0b0011111111111100,
			0b0000000000000000,
		};

		inline constexpr FieldIconBits kAttributeSpeedRate{
			0b0000000000000000,
			0b0000110000000000,
			0b0000111000000000,
			0b0000111100000000,
			0b0000111110000000,
			0b0000111111000000,
			0b0000111111100000,
			0b0000111111110000,
			0b0000111111110000,
			0b0000111111100000,
			0b0000111111000000,
			0b0000111110000000,
			0b0000111100000000,
			0b0000111000000000,
			0b0000110000000000,
			0b0000000000000000,
		};

		inline constexpr FieldIconBits kRuleOrbit{
			0b0000000000000000,
			0b0000011111100000,
			0b0001111111111000,
			0b0011110000111100,
			0b0111000000001110,
			0b0110000000000110,
			0b1110000000000111,
			0b1100000000000011,
			0b1100000000000011,
			0b1110000000000111,
			0b0110000000000110,
			0b0111000000001110,
			0b0011110000111100,
			0b0001111111111000,
			0b0000011111100000,
			0b0000000000000000,
		};

		inline constexpr FieldIconBits kOtherChild{
			0b0000000000000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0011111111111100,
			0b0011111111111100,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000110000000,
			0b0000000000000000,
		};
	}

	inline constexpr std::array<FieldIconBits, AttackNodeLabelCount()> kFieldIcons{
		detail::kSpawnBall,
		detail::kAttributeLifetime,
		detail::kAttributeSpeedRate,
		detail::kRuleOrbit,
		detail::kOtherChild,
	};

	
	// Lookup icon bits for a gameplay label.
	[[nodiscard]] inline const FieldIconBits& GetFieldIcon(AttackNodeLabel id) noexcept
	{
		const std::size_t i = ToIndex(id);
		assert(i < AttackNodeLabelCount());
		if (i >= AttackNodeLabelCount())
		{
			return kFieldIcons[0];
		}
		return kFieldIcons[i];
	}

	
	// Blit 1-bit icon onto canvas (bit0 = left). Clips to canvas size.
	inline void BlitFieldIcon(Canvas& canvas, const FieldIconBits& bits, Color color)
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
				if ((row & static_cast<std::uint16_t>(1u << x)) != 0u)
				{
					canvas.PutPixel(x, y, color);
				}
			}
		}
	}
}
