#include "SettingsStore.h"

#include "GameStatsCodex.h"
#include "JsonTextCopy.h"
#include "json.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
	using json = nlohmann::json;

	std::filesystem::path g_path{};

	void ApplyJsonToCodex_(const json& top)
	{
		if (top.contains("language") && top["language"].is_string())
		{
			const auto lang = ParseLanguageKey(top["language"].get<std::string>());
			if (lang.has_value())
			{
				GameStatsCodex::SetLanguage(*lang);
			}
		}
		if (top.contains("masterVolume") && top["masterVolume"].is_number())
		{
			GameStatsCodex::SetMasterVolume(top["masterVolume"].get<float>());
		}
		if (top.contains("bgmVolume") && top["bgmVolume"].is_number())
		{
			GameStatsCodex::SetBgmVolume(top["bgmVolume"].get<float>());
		}
		if (top.contains("seVolume") && top["seVolume"].is_number())
		{
			GameStatsCodex::SetSeVolume(top["seVolume"].get<float>());
		}
		if (top.contains("muted") && top["muted"].is_boolean())
		{
			GameStatsCodex::SetMuted(top["muted"].get<bool>());
		}
		if (top.contains("fullscreen") && top["fullscreen"].is_boolean())
		{
			GameStatsCodex::SetFullscreen(top["fullscreen"].get<bool>());
		}
		if (top.contains("windowSizeIndex") && top["windowSizeIndex"].is_number_integer())
		{
			GameStatsCodex::SetWindowSizeIndex(top["windowSizeIndex"].get<int>());
		}
	}

	[[nodiscard]] json CodexToJson_()
	{
		json top = json::object();
		top["language"] = ToLanguageKey(GameStatsCodex::GetLanguage());
		top["masterVolume"] = GameStatsCodex::GetMasterVolume();
		top["bgmVolume"] = GameStatsCodex::GetBgmVolume();
		top["seVolume"] = GameStatsCodex::GetSeVolume();
		top["muted"] = GameStatsCodex::GetMuted();
		top["fullscreen"] = GameStatsCodex::GetFullscreen();
		top["windowSizeIndex"] = GameStatsCodex::GetWindowSizeIndex();
		return top;
	}

	bool LoadSettingsFromPath_(const std::filesystem::path& path)
	{
		json top;
		if (!ReadCopyJson(path, top))
		{
			return false;
		}
		ApplyJsonToCodex_(top);
		g_path = path;
		return true;
	}

	bool WriteSettingsToPath_(const std::filesystem::path& path)
	{
		std::error_code ec;
		const std::filesystem::path parent = path.parent_path();
		if (!parent.empty())
		{
			std::filesystem::create_directories(parent, ec);
			if (ec)
			{
				return false;
			}
		}
		std::ofstream out(path);
		if (!out.is_open())
		{
			return false;
		}
		try
		{
			out << CodexToJson_().dump(2);
			out << '\n';
		}
		catch (...)
		{
			return false;
		}
		if (!out.good())
		{
			return false;
		}
		g_path = path;
		return true;
	}

	[[nodiscard]] std::filesystem::path SavesSettingsRel_()
	{
		return std::filesystem::path("Saves") / "settings.json";
	}
}

bool LoadSettings()
{
	return TryLoadCopyWithFallback("Saves/settings.json", static_cast<bool(*)(const std::filesystem::path&)>(&LoadSettingsFromPath_));
}

bool SaveSettings()
{
	const std::filesystem::path rel = SavesSettingsRel_();
	const std::filesystem::path candidates[] = {
		rel,
		std::filesystem::path("CoreRefiner") / rel,
		std::filesystem::path("CoreRefiner") / "CoreRefiner" / rel,
	};
	if (!g_path.empty())
	{
		for (const std::filesystem::path& candidate : candidates)
		{
			if (g_path == candidate && WriteSettingsToPath_(g_path))
			{
				return true;
			}
		}
	}
	for (const std::filesystem::path& candidate : candidates)
	{
		if (WriteSettingsToPath_(candidate))
		{
			return true;
		}
	}
	return false;
}
