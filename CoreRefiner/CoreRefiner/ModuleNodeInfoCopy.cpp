#include "ModuleNodeInfoCopy.h"

#include "GameStatsCodex.h"
#include "IModuleNode.h"
#include "ModuleNodes.h"
#include "Util.h"
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
		// ————————————————————————————————————————————————————
		// Kind —— Core
		// ————————————————————————————————————————————————————
		if (name == "Core_Ball")				{ return ModuleNodeLabel::Core_Ball; }
		// ————————————————————————————————————————————————————
		// Kind —— Spawn
		// ————————————————————————————————————————————————————
		if (name == "Spawn_Ball")				{ return ModuleNodeLabel::Spawn_Ball; }
		// ————————————————————————————————————————————————————
		// Kind —— Attribute
		// ————————————————————————————————————————————————————
		if (name == "Attribute_LifetimeRate")	{ return ModuleNodeLabel::Attribute_LifetimeRate; }
		if (name == "Attribute_SpeedRate")		{ return ModuleNodeLabel::Attribute_SpeedRate; }
		if (name == "Attribute_SizeRate")		{ return ModuleNodeLabel::Attribute_SizeRate; }
		if (name == "Attribute_DamageRate")		{ return ModuleNodeLabel::Attribute_DamageRate; }
		// ————————————————————————————————————————————————————
		// Kind —— Rule
		// ————————————————————————————————————————————————————
		if (name == "Rule_Orbit")				{ return ModuleNodeLabel::Rule_Orbit; }
		if (name == "Rule_Return")				{ return ModuleNodeLabel::Rule_Return; }
		if (name == "Rule_Child")				{ return ModuleNodeLabel::Rule_Child; }
		if (name == "Rule_Revive")				{ return ModuleNodeLabel::Rule_Revive; }
		// ————————————————————————————————————————————————————
		// Kind —— Passive
		// ————————————————————————————————————————————————————
		if (name == "Passive_DamageFix")		{ return ModuleNodeLabel::Passive_DamageFix; }
		// ————————————————————————————————————————————————————
		// Kind —— Other
		// ————————————————————————————————————————————————————
		if (name == "Other_Repeat")				{ return ModuleNodeLabel::Other_Repeat; }
		// ————————————————————————————————————————————————————
		// Kind —— Fusion
		// ————————————————————————————————————————————————————
		if (name == "Fusion")					{ return ModuleNodeLabel::Fusion; }
		// ————————————————————————————————————————————————————
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
	// ————————————————————————————————————————————————————
	// Kind —— Core
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Core_Ball:				return "Core_Ball";
	// ————————————————————————————————————————————————————
	// Kind —— Spawn
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Spawn_Ball:				return "Spawn_Ball";
	// ————————————————————————————————————————————————————
	// Kind —— Attribute
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Attribute_LifetimeRate:	return "Attribute_LifetimeRate";
	case ModuleNodeLabel::Attribute_SpeedRate:		return "Attribute_SpeedRate";
	case ModuleNodeLabel::Attribute_SizeRate:		return "Attribute_SizeRate";
	case ModuleNodeLabel::Attribute_DamageRate:		return "Attribute_DamageRate";
	// ————————————————————————————————————————————————————
	// Kind —— Rule
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Rule_Orbit:				return "Rule_Orbit";
	case ModuleNodeLabel::Rule_Return:				return "Rule_Return";
	case ModuleNodeLabel::Rule_Child:				return "Rule_Child";
	case ModuleNodeLabel::Rule_Revive:				return "Rule_Revive";
	// ————————————————————————————————————————————————————
	// Kind —— Passive
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Passive_DamageFix:		return "Passive_DamageFix";
	// ————————————————————————————————————————————————————
	// Kind —— Other
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Other_Repeat:				return "Other_Repeat";
	// ————————————————————————————————————————————————————
	// Kind —— Fusion
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Fusion:					return "Fusion";
	// ————————————————————————————————————————————————————
	case ModuleNodeLabel::Count:					return "";
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

bool LoadModuleNodeInfoCopy()
{
	if (g_loaded)
	{
		return true;
	}
	return TryLoadCopyWithFallback("ModuleNodeInfoCopy.json", static_cast<bool(*)(const std::filesystem::path&)>(&LoadModuleNodeInfoCopy));
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

namespace
{
	// 只搬落在 title 里的 span（当前 JSON 都是标题加粗）。
	void AppendTitleSpans_(std::vector<Text::Span>& out, const ModuleNodeInfoEntry& entry, UINT32 titleOffset)
	{
		const UINT32 titleUnits = static_cast<UINT32>(Utf16CodeUnitCount(entry.title));
		for (const Text::Span& sp : entry.spans)
		{
			if (sp.length == 0u)
			{
				continue;
			}
			if (sp.start + sp.length > titleUnits)
			{
				continue;
			}
			Text::Span shifted = sp;
			shifted.start += titleOffset;
			out.push_back(shifted);
		}
	}
}

ModuleNodeInfoEntry ComposeModuleNodeInfoCopy(const IModuleNode& node)
{
	if (node.GetKind() != ModuleNodeKind::Fusion)
	{
		return GetModuleNodeInfoCopy(node.GetModuleNodeLabel());
	}

	const auto& fusion = static_cast<const ModuleNode_Fusion&>(node);
	const IModuleNode* primary = fusion.GetPrimary();
	const IModuleNode* material = fusion.GetMaterial();
	if (primary == nullptr || material == nullptr)
	{
		return GetModuleNodeInfoCopy(ModuleNodeLabel::Fusion);
	}

	const ModuleNodeInfoEntry primaryCopy = ComposeModuleNodeInfoCopy(*primary);
	const ModuleNodeInfoEntry materialCopy = ComposeModuleNodeInfoCopy(*material);

	ModuleNodeInfoEntry out{};
	out.title = primaryCopy.title + " & " + materialCopy.title;
	if (primaryCopy.body.empty())
	{
		out.body = materialCopy.body;
	}
	else if (materialCopy.body.empty())
	{
		out.body = primaryCopy.body;
	}
	else
	{
		out.body = primaryCopy.body + "\n" + materialCopy.body;
	}

	const UINT32 materialTitleAt = static_cast<UINT32>(Utf16CodeUnitCount(primaryCopy.title) + Utf16CodeUnitCount(" & "));
	AppendTitleSpans_(out.spans, primaryCopy, 0u);
	AppendTitleSpans_(out.spans, materialCopy, materialTitleAt);
	return out;
}