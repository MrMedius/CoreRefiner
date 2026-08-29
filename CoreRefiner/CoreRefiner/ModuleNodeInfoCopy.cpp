#include "ModuleNodeInfoCopy.h"

#include "GameStatsCodex.h"
#include "json.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace
{
	using json = nlohmann::json;

	bool g_loaded{ false };

	using LabelTable = std::unordered_map<std::size_t, ModuleNodeInfoEntry>;
	std::array<LabelTable, LanguageCount()> g_tables{};

	[[nodiscard]] const ModuleNodeInfoEntry& BakedDefault_()
	{
		static const ModuleNodeInfoEntry kDefault{
			"Unknown Module",
			"No localized copy is available for this node.",
			{}
		};
		return kDefault;
	}

	[[nodiscard]] std::optional<ModuleNodeLabel> ParseModuleNodeLabel_(std::string_view name) noexcept
	{
		if (name == "Core_Ball")
		{
			return ModuleNodeLabel::Core_Ball;
		}
		if (name == "Spawn_Ball")
		{
			return ModuleNodeLabel::Spawn_Ball;
		}
		if (name == "Attribute_Lifetime")
		{
			return ModuleNodeLabel::Attribute_Lifetime;
		}
		if (name == "Attribute_SpeedRate")
		{
			return ModuleNodeLabel::Attribute_SpeedRate;
		}
		if (name == "Attribute_SizeRate")
		{
			return ModuleNodeLabel::Attribute_SizeRate;
		}
		if (name == "Attribute_DamageRate")
		{
			return ModuleNodeLabel::Attribute_DamageRate;
		}
		if (name == "Rule_Orbit")
		{
			return ModuleNodeLabel::Rule_Orbit;
		}
		if (name == "Rule_Return")
		{
			return ModuleNodeLabel::Rule_Return;
		}
		if (name == "Passive_DamageFix")
		{
			return ModuleNodeLabel::Passive_DamageFix;
		}
		if (name == "Other_Child")
		{
			return ModuleNodeLabel::Other_Child;
		}
		if (name == "Other_Revive")
		{
			return ModuleNodeLabel::Other_Revive;
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

	[[nodiscard]] ModuleNodeInfoEntry ParseEntry_(const json& j)
	{
		ModuleNodeInfoEntry entry{};
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

	[[nodiscard]] const ModuleNodeInfoEntry* TryGet_(
		Language lang,
		ModuleNodeLabel label) noexcept
	{
		if (label == ModuleNodeLabel::Count)
		{
			return nullptr;
		}
		const LabelTable& table = g_tables[ToIndex(lang)];
		const auto it = table.find(ToIndex(label));
		if (it == table.end())
		{
			return nullptr;
		}
		return &it->second;
	}
}

const char* ToModuleNodeLabelName(ModuleNodeLabel label) noexcept
{
	switch (label)
	{
	case ModuleNodeLabel::Core_Ball: return "Core_Ball";
	case ModuleNodeLabel::Spawn_Ball: return "Spawn_Ball";
	case ModuleNodeLabel::Attribute_Lifetime: return "Attribute_Lifetime";
	case ModuleNodeLabel::Attribute_SpeedRate: return "Attribute_SpeedRate";
	case ModuleNodeLabel::Attribute_SizeRate: return "Attribute_SizeRate";
	case ModuleNodeLabel::Attribute_DamageRate: return "Attribute_DamageRate";
	case ModuleNodeLabel::Rule_Orbit: return "Rule_Orbit";
	case ModuleNodeLabel::Rule_Return: return "Rule_Return";
	case ModuleNodeLabel::Passive_DamageFix: return "Passive_DamageFix";
	case ModuleNodeLabel::Other_Child: return "Other_Child";
	case ModuleNodeLabel::Other_Revive: return "Other_Revive";
	case ModuleNodeLabel::Count: return "";
	}
	return "";
}

bool LoadModuleNodeInfoCopy(const std::filesystem::path& path)
{
	for (auto& table : g_tables)
	{
		table.clear();
	}
	g_loaded = false;

	json top;
	if (!ReadCopyJson(path, top))
	{
		return false;
	}

	bool any = false;
	ForEachLanguageObject(top, [&](Language lang, const json& langObj)
	{
		LabelTable& table = g_tables[ToIndex(lang)];
		for (auto lit = langObj.begin(); lit != langObj.end(); ++lit)
		{
			const auto labelOpt = ParseModuleNodeLabel_(lit.key());
			if (!labelOpt.has_value() || !lit.value().is_object())
			{
				continue;
			}
			table[ToIndex(*labelOpt)] = ParseEntry_(lit.value());
			any = true;
		}
	});

	g_loaded = any;
	return any;
}

bool IsModuleNodeInfoCopyLoaded() noexcept
{
	return g_loaded;
}

const ModuleNodeInfoEntry& GetModuleNodeInfoCopy(ModuleNodeLabel label)
{
	const Language order[] = {
		GameStatsCodex::GetLanguage(),
		Language::En,
		Language::Zh,
	};

	for (Language lang : order)
	{
		if (const ModuleNodeInfoEntry* e = TryGet_(lang, label))
		{
			return *e;
		}
	}
	return BakedDefault_();
}
