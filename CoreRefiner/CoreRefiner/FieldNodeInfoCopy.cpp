#include "FieldNodeInfoCopy.h"

#include "json.hpp"

#include <array>
#include <fstream>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace
{
	using json = nlohmann::json;

	FieldNodeInfoLanguage g_language{ FieldNodeInfoLanguage::Zh };
	bool g_loaded{ false };

	/** @brief Per-language map: AttackNodeLabel index → entry. */
	using LabelTable = std::unordered_map<std::size_t, FieldNodeInfoEntry>;
	std::array<LabelTable, 3> g_tables{};

	[[nodiscard]] constexpr std::size_t LangIndex(FieldNodeInfoLanguage lang) noexcept
	{
		return static_cast<std::size_t>(lang);
	}

	[[nodiscard]] const FieldNodeInfoEntry& BakedDefault_()
	{
		static const FieldNodeInfoEntry kDefault{
			"Unknown Module",
			"No localized copy is available for this node.",
			{}
		};
		return kDefault;
	}

	[[nodiscard]] std::optional<AttackNodeLabel> ParseAttackNodeLabel_(std::string_view name) noexcept
	{
		if (name == "Spawn_Ball")
		{
			return AttackNodeLabel::Spawn_Ball;
		}
		if (name == "Attribute_Lifetime")
		{
			return AttackNodeLabel::Attribute_Lifetime;
		}
		if (name == "Attribute_SpeedRate")
		{
			return AttackNodeLabel::Attribute_SpeedRate;
		}
		if (name == "Rule_Orbit")
		{
			return AttackNodeLabel::Rule_Orbit;
		}
		if (name == "Other_Child")
		{
			return AttackNodeLabel::Other_Child;
		}
		return std::nullopt;
	}

	[[nodiscard]] std::optional<FieldNodeInfoLanguage> ParseLanguageKey_(std::string_view key) noexcept
	{
		if (key == "zh")
		{
			return FieldNodeInfoLanguage::Zh;
		}
		if (key == "ja")
		{
			return FieldNodeInfoLanguage::Ja;
		}
		if (key == "en")
		{
			return FieldNodeInfoLanguage::En;
		}
		return std::nullopt;
	}

	[[nodiscard]] DWRITE_FONT_WEIGHT ParseWeight_(std::string_view w) noexcept
	{
		if (w == "bold" || w == "Bold")
		{
			return DWRITE_FONT_WEIGHT_BOLD;
		}
		if (w == "light" || w == "Light")
		{
			return DWRITE_FONT_WEIGHT_LIGHT;
		}
		if (w == "medium" || w == "Medium")
		{
			return DWRITE_FONT_WEIGHT_MEDIUM;
		}
		if (w == "semiBold" || w == "semibold" || w == "SemiBold")
		{
			return DWRITE_FONT_WEIGHT_SEMI_BOLD;
		}
		return DWRITE_FONT_WEIGHT_NORMAL;
	}

	[[nodiscard]] FieldNodeInfoEntry ParseEntry_(const json& j)
	{
		FieldNodeInfoEntry entry{};
		if (j.contains("title") && j["title"].is_string())
		{
			entry.title = j["title"].get<std::string>();
		}
		if (j.contains("body") && j["body"].is_string())
		{
			entry.body = j["body"].get<std::string>();
		}
		if (j.contains("spans") && j["spans"].is_array())
		{
			for (const json& s : j["spans"])
			{
				if (!s.is_object())
				{
					continue;
				}
				Text::Span span{};
				span.start = s.value("start", 0u);
				span.length = s.value("length", 0u);
				if (s.contains("r") || s.contains("g") || s.contains("b") || s.contains("a"))
				{
					const unsigned char r = static_cast<unsigned char>(s.value("r", 255));
					const unsigned char g = static_cast<unsigned char>(s.value("g", 255));
					const unsigned char b = static_cast<unsigned char>(s.value("b", 255));
					const unsigned char a = static_cast<unsigned char>(s.value("a", 255));
					span.color = Color{ r, g, b, a };
				}
				if (s.contains("weight") && s["weight"].is_string())
				{
					span.weight = ParseWeight_(s["weight"].get<std::string>());
				}
				if (s.contains("underline") && s["underline"].is_boolean())
				{
					span.underline = s["underline"].get<bool>();
				}
				entry.spans.push_back(std::move(span));
			}
		}
		return entry;
	}

	[[nodiscard]] const FieldNodeInfoEntry* TryGet_(
		FieldNodeInfoLanguage lang,
		AttackNodeLabel label) noexcept
	{
		if (label == AttackNodeLabel::Count)
		{
			return nullptr;
		}
		const LabelTable& table = g_tables[LangIndex(lang)];
		const auto it = table.find(ToIndex(label));
		if (it == table.end())
		{
			return nullptr;
		}
		return &it->second;
	}
}

void SetFieldNodeInfoLanguage(FieldNodeInfoLanguage lang) noexcept
{
	g_language = lang;
}

FieldNodeInfoLanguage GetFieldNodeInfoLanguage() noexcept
{
	return g_language;
}

const char* ToAttackNodeLabelName(AttackNodeLabel label) noexcept
{
	switch (label)
	{
	case AttackNodeLabel::Spawn_Ball: return "Spawn_Ball";
	case AttackNodeLabel::Attribute_Lifetime: return "Attribute_Lifetime";
	case AttackNodeLabel::Attribute_SpeedRate: return "Attribute_SpeedRate";
	case AttackNodeLabel::Rule_Orbit: return "Rule_Orbit";
	case AttackNodeLabel::Other_Child: return "Other_Child";
	case AttackNodeLabel::Count: return "";
	}
	return "";
}

const char* ToFieldNodeInfoLanguageKey(FieldNodeInfoLanguage lang) noexcept
{
	switch (lang)
	{
	case FieldNodeInfoLanguage::Zh: return "zh";
	case FieldNodeInfoLanguage::Ja: return "ja";
	case FieldNodeInfoLanguage::En: return "en";
	}
	return "zh";
}

bool LoadFieldNodeInfoCopy(const std::filesystem::path& path)
{
	for (auto& table : g_tables)
	{
		table.clear();
	}
	g_loaded = false;

	std::ifstream in(path);
	if (!in.is_open())
	{
		return false;
	}

	json top;
	try
	{
		in >> top;
	}
	catch (...)
	{
		return false;
	}

	if (!top.is_object())
	{
		return false;
	}

	bool any = false;
	for (auto it = top.begin(); it != top.end(); ++it)
	{
		const auto langOpt = ParseLanguageKey_(it.key());
		if (!langOpt.has_value() || !it.value().is_object())
		{
			continue;
		}

		LabelTable& table = g_tables[LangIndex(*langOpt)];
		for (auto lit = it.value().begin(); lit != it.value().end(); ++lit)
		{
			const auto labelOpt = ParseAttackNodeLabel_(lit.key());
			if (!labelOpt.has_value() || !lit.value().is_object())
			{
				continue;
			}
			table[ToIndex(*labelOpt)] = ParseEntry_(lit.value());
			any = true;
		}
	}

	g_loaded = any;
	return any;
}

bool IsFieldNodeInfoCopyLoaded() noexcept
{
	return g_loaded;
}

const FieldNodeInfoEntry& GetFieldNodeInfoCopy(AttackNodeLabel label)
{
	const FieldNodeInfoLanguage order[] = {
		g_language,
		FieldNodeInfoLanguage::En,
		FieldNodeInfoLanguage::Zh,
	};

	for (FieldNodeInfoLanguage lang : order)
	{
		if (const FieldNodeInfoEntry* e = TryGet_(lang, label))
		{
			return *e;
		}
	}
	return BakedDefault_();
}
