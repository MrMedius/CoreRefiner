#pragma once
#include "ModuleNodeLabel.h"
#include "Canvas.h"
#include "Colors.h"
#include "GameRulesCodex.h"

#include <array>
#include <cassert>
#include <cstdint>

namespace IconAtlas
{
	using IconBits = std::array<std::uint16_t, 16>;

	// x 半开区间 [x0, x1)；默认整图。Fusion 左半 0–8、右半 8–16。
	inline void BlitIcon(Canvas& canvas, const IconBits& bits, Color color, unsigned x0 = 0u, unsigned x1 = 16u)
	{
		const unsigned w = canvas.GetCanvasWidth();
		const unsigned h = canvas.GetCanvasHeight();
		const unsigned rows = (h < 16u) ? h : 16u;
		const unsigned cols = (w < 16u) ? w : 16u;
		const unsigned xBegin = (x0 < cols) ? x0 : cols;
		const unsigned xEnd = (x1 < cols) ? x1 : cols;

		for (unsigned y = 0u; y < rows; ++y)
		{
			const std::uint16_t row = bits[y];
			for (unsigned x = xBegin; x < xEnd; ++x)
			{
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) != 0u)
				{
					canvas.PutPixel(x, y, color);
				}
			}
		}
	}

	// 色相 [0,1) 转纯色。0 是红，沿斜向递增。
	[[nodiscard]] inline Color HueToRgb(float hue) noexcept
	{
		hue = hue - static_cast<float>(static_cast<int>(hue));
		if (hue < 0.0f)
		{
			hue += 1.0f;
		}
		const float scaled = hue * 6.0f;
		const int sector = static_cast<int>(scaled);
		const float frac = scaled - static_cast<float>(sector);
		const auto up = static_cast<unsigned char>(frac * 255.0f);
		const auto down = static_cast<unsigned char>((1.0f - frac) * 255.0f);
		switch (sector)
		{
		case 0: return Color(255u, up, 0u);
		case 1: return Color(down, 255u, 0u);
		case 2: return Color(0u, 255u, up);
		case 3: return Color(0u, down, 255u);
		case 4: return Color(up, 0u, 255u);
		default: return Color(255u, 0u, down);
		}
	}

	// 亮像素按 x+y+phase 取斜向色相。yOffset 是写到画布上的行起点，色相仍用图标自己的 y。
	inline void BlitIconRainbow(
		Canvas& canvas,
		const IconBits& bits,
		float phase,
		unsigned x0 = 0u,
		unsigned x1 = 16u,
		unsigned yOffset = 0u)
	{
		const unsigned w = canvas.GetCanvasWidth();
		const unsigned h = canvas.GetCanvasHeight();
		const unsigned cols = (w < 16u) ? w : 16u;
		const unsigned xBegin = (x0 < cols) ? x0 : cols;
		const unsigned xEnd = (x1 < cols) ? x1 : cols;

		for (unsigned y = 0u; y < 16u; ++y)
		{
			const unsigned dstY = yOffset + y;
			if (dstY >= h)
			{
				break;
			}
			const std::uint16_t row = bits[y];
			for (unsigned x = xBegin; x < xEnd; ++x)
			{
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) == 0u)
				{
					continue;
				}
				const float hue = (static_cast<float>(x + y) + phase) / GameRulesCodex::RainbowPeriod;
				canvas.PutPixel(x, dstY, HueToRgb(hue));
			}
		}
	}

	// N 帧竖排。第 f 帧的相位沿斜向错开，一整张图走完一圈。
	inline void BakeRainbowSheet(Canvas& canvas, const IconBits& bits, unsigned frames)
	{
		if (frames == 0u)
		{
			return;
		}
		for (unsigned f = 0u; f < frames; ++f)
		{
			const float phase = GameRulesCodex::RainbowPeriod * static_cast<float>(f) / static_cast<float>(frames);
			BlitIconRainbow(canvas, bits, phase, 0u, 16u, f * 16u);
		}
	}
}

namespace NodeIconAtlas
{
	namespace detail
	{
		// ————————————————————————————————————————————————————
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
		// ————————————————————————————————————————————————————
		// Kind —— Core
		// ————————————————————————————————————————————————————
		inline constexpr IconAtlas::IconBits kCoreBall{
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
		// ————————————————————————————————————————————————————
		// Kind —— Spawn
		// ————————————————————————————————————————————————————
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
		// ————————————————————————————————————————————————————
		// Kind —— Attribute
		// ————————————————————————————————————————————————————
		inline constexpr IconAtlas::IconBits kAttributeLifetimeRate{
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
		// ————————————————————————————————————————————————————
		// Kind —— Rule
		// ————————————————————————————————————————————————————
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
		inline constexpr IconAtlas::IconBits kRuleChild{
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
		inline constexpr IconAtlas::IconBits kRuleRevive{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010100110010100,
			0b0100010110100010,
			0b0100000110000010,
			0b1000111111110001,
			0b1000111111110001,
			0b1000000110000001,
			0b1000010110100001,
			0b0100100110010010,
			0b0101000110001010,
			0b0010000110000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};
		// ————————————————————————————————————————————————————
		// Kind —— Element
		// ————————————————————————————————————————————————————
		// 还没有图案。先放一张空图，让这个标签在表里有自己的一格。
		inline constexpr IconAtlas::IconBits kElementYin{};
		// ————————————————————————————————————————————————————
		// Kind —— Passive
		// ————————————————————————————————————————————————————
		inline constexpr IconAtlas::IconBits kPassiveDamageFix{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010000000100100,
			0b0100000000100010,
			0b0100000011111010,
			0b1000000000100001,
			0b1000001110100001,
			0b1000000110000001,
			0b1000101010000001,
			0b0100010000000010,
			0b0100101000000010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};
		// ————————————————————————————————————————————————————
		// Kind —— Other
		// ————————————————————————————————————————————————————
		inline constexpr IconAtlas::IconBits kOtherRepeat{
			0b0000001111000000,
			0b0000110000110000,
			0b0001000000001000,
			0b0010001110000100,
			0b0100010001010010,
			0b0100100000110010,
			0b1000100001110001,
			0b1000000000000001,
			0b1000110000110001,
			0b1001001001001001,
			0b0101001001001010,
			0b0100110000110010,
			0b0010000000000100,
			0b0001000000001000,
			0b0000110000110000,
			0b0000001111000000,
		};
		// ————————————————————————————————————————————————————
		// Kind —— Fusion
		// ————————————————————————————————————————————————————
		// 融合没有自己的图案。绘制走主体和素材各一半。
		inline constexpr IconAtlas::IconBits kFusion{};
		// ————————————————————————————————————————————————————
		// Kind —— Ultra
		// ————————————————————————————————————————————————————
		// 图集槽用圆框。实例图案以后存在奥义自己身上。
		inline constexpr IconAtlas::IconBits kUltra = kRoundFrame;
		// 查不到标签时返回这张空图。
		inline constexpr IconAtlas::IconBits kBlank{};
	}

	// 一张节点图。label 是它对应的标签，bits 指向图案。取图认标签，不认这张表的先后。
	struct NodeIconSlot
	{
		ModuleNodeLabel label;
		const IconAtlas::IconBits* bits;
	};

	inline constexpr NodeIconSlot kNodeIcons[] = {
		// ————————————————————————————————————————————————————
		// Kind —— Core
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Core_Ball, &detail::kCoreBall },
		// ————————————————————————————————————————————————————
		// Kind —— Spawn
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Spawn_Ball, &detail::kSpawnBall },
		// ————————————————————————————————————————————————————
		// Kind —— Attribute
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Attribute_LifetimeRate, &detail::kAttributeLifetimeRate },
		{ ModuleNodeLabel::Attribute_SpeedRate, &detail::kAttributeSpeedRate },
		{ ModuleNodeLabel::Attribute_SizeRate, &detail::kAttributeSizeRate },
		{ ModuleNodeLabel::Attribute_DamageRate, &detail::kAttributeDamageRate },
		// ————————————————————————————————————————————————————
		// Kind —— Rule
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Rule_Orbit, &detail::kRuleOrbit },
		{ ModuleNodeLabel::Rule_Return, &detail::kRuleReturn },
		{ ModuleNodeLabel::Rule_Child, &detail::kRuleChild },
		{ ModuleNodeLabel::Rule_Revive, &detail::kRuleRevive },
		// ————————————————————————————————————————————————————
		// Kind —— Element
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Element_Yin, &detail::kElementYin },
		// ————————————————————————————————————————————————————
		// Kind —— Passive
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Passive_DamageFix, &detail::kPassiveDamageFix },
		// ————————————————————————————————————————————————————
		// Kind —— Other
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Other_Repeat, &detail::kOtherRepeat },
		// ————————————————————————————————————————————————————
		// Kind —— Fusion
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Fusion, &detail::kFusion },
		// ————————————————————————————————————————————————————
		// Kind —— Ultra
		// ————————————————————————————————————————————————————
		{ ModuleNodeLabel::Ultra, &detail::kUltra },
	};

	// 每个标签恰好有一张图。漏一张或写重一张时，编译失败。
	[[nodiscard]] inline constexpr bool NodeIconsCoverLabels() noexcept
	{
		constexpr std::size_t count = sizeof(kNodeIcons) / sizeof(kNodeIcons[0]);
		if (count != ModuleNodeLabelCount())
		{
			return false;
		}
		for (std::size_t i = 0; i < ModuleNodeLabelCount(); ++i)
		{
			const auto label = static_cast<ModuleNodeLabel>(i);
			std::size_t hits = 0;
			for (std::size_t n = 0; n < count; ++n)
			{
				if (kNodeIcons[n].label == label && kNodeIcons[n].bits != nullptr)
				{
					++hits;
				}
			}
			if (hits != 1u)
			{
				return false;
			}
		}
		return true;
	}
	static_assert(NodeIconsCoverLabels());

	// 按标签取图案。对不上时返回空图。
	[[nodiscard]] inline const IconAtlas::IconBits& Get(ModuleNodeLabel id) noexcept
	{
		for (const NodeIconSlot& slot : kNodeIcons)
		{
			if (slot.label == id && slot.bits != nullptr)
			{
				return *slot.bits;
			}
		}
		return detail::kBlank;
	}
}

enum class UiIconId : unsigned char
{
	Currency,
	Refresh,
	LockOpen,
	LockClosed,
	RefineFeed,
	RefineYield,
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
		inline constexpr IconAtlas::IconBits kRefineFeed{
			0b0000000110000000,
			0b0000000011000000,
			0b0000000001100000,
			0b0000000000110000,
			0b0000000000011000,
			0b0000000000001100,
			0b0000000000000110,
			0b1111111111111111,
			0b1111111111111111,
			0b0000000000000110,
			0b0000000000001100,
			0b0000000000011000,
			0b0000000000110000,
			0b0000000001100000,
			0b0000000011000000,
			0b0000000110000000,
		};
		inline constexpr IconAtlas::IconBits kRefineYield{
			0b0000001110000000,
			0b0000000111000000,
			0b0000000011100000,
			0b0000000001110000,
			0b0000000000111000,
			0b1111111111111100,
			0b1111111111111110,
			0b0000000000001111,
			0b0000000000001111,
			0b1111111111111110,
			0b1111111111111100,
			0b0000000000111000,
			0b0000000001110000,
			0b0000000011100000,
			0b0000000111000000,
			0b0000001110000000,
		};
	}

	inline constexpr std::array<IconAtlas::IconBits, UiIconIdCount()> kUiIcons{
		detail::kCurrency,
		detail::kRefresh,
		detail::kLockOpen,
		detail::kLockClosed,
		detail::kRefineFeed,
		detail::kRefineYield,
	};

	static_assert(kUiIcons.size() == UiIconIdCount());

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