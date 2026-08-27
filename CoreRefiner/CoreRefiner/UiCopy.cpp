#include "UiCopy.h"

#include "GameStatsCodex.h"
#include "JsonTextCopy.h"
#include "json.hpp"

#include <array>
#include <string>
#include <unordered_map>

namespace
{
	using json = nlohmann::json;

	bool g_loaded{ false };

	using CopyTable = std::unordered_map<std::string, std::string>;
	std::array<CopyTable, LanguageCount()> g_tables{};

	[[nodiscard]] const std::string* TryGet_(Language lang, const std::string& key) noexcept
	{
		const CopyTable& table = g_tables[ToIndex(lang)];
		const auto it = table.find(key);
		if (it == table.end())
		{
			return nullptr;
		}
		return &it->second;
	}

	[[nodiscard]] std::string ReplaceArg0_(std::string text, std::string_view arg0)
	{
		const auto pos = text.find("{0}");
		if (pos != std::string::npos)
		{
			text.replace(pos, 3, arg0);
		}
		return text;
	}
}

bool LoadUiCopy(const std::filesystem::path& path)
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
		CopyTable& table = g_tables[ToIndex(lang)];
		for (auto kit = langObj.begin(); kit != langObj.end(); ++kit)
		{
			if (!kit.value().is_string())
			{
				continue;
			}
			table[kit.key()] = kit.value().get<std::string>();
			any = true;
		}
	});

	g_loaded = any;
	return any;
}

bool LoadUiCopy()
{
	if (g_loaded)
	{
		return true;
	}
	return TryLoadCopyWithFallback(
		"UiCopy.json",
		static_cast<bool(*)(const std::filesystem::path&)>(&LoadUiCopy));
}

bool IsUiCopyLoaded() noexcept
{
	return g_loaded;
}

std::string GetUiCopy(std::string_view key)
{
	const std::string keyStr{ key };
	const Language order[] = {
		GameStatsCodex::GetLanguage(),
		Language::En,
		Language::Zh,
	};

	for (Language lang : order)
	{
		if (const std::string* text = TryGet_(lang, keyStr))
		{
			return *text;
		}
	}
	return keyStr;
}

std::string GetUiCopy(std::string_view key, std::string_view arg0)
{
	return ReplaceArg0_(GetUiCopy(key), arg0);
}

std::string GetUiCopy(std::string_view key, int arg0)
{
	const std::string arg = std::to_string(arg0);
	return GetUiCopy(key, std::string_view{ arg });
}
