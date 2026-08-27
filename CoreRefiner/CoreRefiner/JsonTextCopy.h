#pragma once
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

enum class Language : unsigned char
{
	Zh,
	Ja,
	En,
};

[[nodiscard]] constexpr std::size_t LanguageCount() noexcept
{
	return 3u;
}

[[nodiscard]] constexpr std::size_t ToIndex(Language lang) noexcept
{
	return static_cast<std::size_t>(lang);
}

[[nodiscard]] inline std::optional<Language> ParseLanguageKey(std::string_view key) noexcept
{
	if (key == "zh")
	{
		return Language::Zh;
	}
	if (key == "ja")
	{
		return Language::Ja;
	}
	if (key == "en")
	{
		return Language::En;
	}
	return std::nullopt;
}

[[nodiscard]] inline const char* ToLanguageKey(Language lang) noexcept
{
	switch (lang)
	{
	case Language::Zh: return "zh";
	case Language::Ja: return "ja";
	case Language::En: return "en";
	}
	return "zh";
}

// 依次尝试 fileName、CoreRefiner/fileName、CoreRefiner/CoreRefiner/fileName。
inline bool TryLoadCopyWithFallback(std::string_view fileName, bool (*loadPath)(const std::filesystem::path&))
{
	if (loadPath == nullptr)
	{
		return false;
	}
	const std::filesystem::path name{ fileName };
	if (loadPath(name))
	{
		return true;
	}
	if (loadPath(std::filesystem::path("CoreRefiner") / name))
	{
		return true;
	}
	return loadPath(std::filesystem::path("CoreRefiner") / "CoreRefiner" / name);
}

// 打开并解析 JSON 对象。失败返回 false。
// Json 须为 nlohmann::json；调用方 cpp 先包含 json.hpp。
template<typename Json>
bool ReadCopyJson(const std::filesystem::path& path, Json& out)
{
	std::ifstream in(path);
	if (!in.is_open())
	{
		return false;
	}
	try
	{
		in >> out;
	}
	catch (...)
	{
		return false;
	}
	return out.is_object();
}

// 遍历顶层 zh/ja/en 对象，对每个语言表调用 fn(lang, object)。
template<typename Json, typename Fn>
bool ForEachLanguageObject(const Json& top, Fn&& fn)
{
	if (!top.is_object())
	{
		return false;
	}
	bool any = false;
	for (auto it = top.begin(); it != top.end(); ++it)
	{
		const auto langOpt = ParseLanguageKey(it.key());
		if (!langOpt.has_value() || !it.value().is_object())
		{
			continue;
		}
		fn(*langOpt, it.value());
		any = true;
	}
	return any;
}