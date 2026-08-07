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
		inline constexpr FieldIconBits kRoundFrame{
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

		inline constexpr FieldIconBits kSpawnBall{
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

		inline constexpr FieldIconBits kAttributeLifetime{
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

		inline constexpr FieldIconBits kAttributeSpeedRate{
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

		inline constexpr FieldIconBits kRuleOrbit{
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

		inline constexpr FieldIconBits kOtherChild{
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
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) != 0u)
				{
					canvas.PutPixel(x, y, color);
				}
			}
		}
	}
}
