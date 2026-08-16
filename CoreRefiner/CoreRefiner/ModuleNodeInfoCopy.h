#pragma once
#include "ModuleNodeLabel.h"
#include "TextTypes.h"

#include <filesystem>
#include <string>
#include <vector>


enum class ModuleNodeInfoLanguage : unsigned char
{
	Zh,
	Ja,
	En,
};

struct ModuleNodeInfoEntry
{
	std::string title;
	std::string body;
	std::vector<Text::Span> spans;

	[[nodiscard]] std::string ComposedText() const
	{
		if (title.empty())
		{
			return body;
		}
		if (body.empty())
		{
			return title;
		}
		return title + "\n" + body;
	}
};

void SetModuleNodeInfoLanguage(ModuleNodeInfoLanguage lang) noexcept;

[[nodiscard]] ModuleNodeInfoLanguage GetModuleNodeInfoLanguage() noexcept;

bool LoadModuleNodeInfoCopy(const std::filesystem::path& path);

[[nodiscard]] bool IsModuleNodeInfoCopyLoaded() noexcept;

[[nodiscard]] const ModuleNodeInfoEntry& GetModuleNodeInfoCopy(ModuleNodeLabel label);

[[nodiscard]] const char* ToModuleNodeLabelName(ModuleNodeLabel label) noexcept;

[[nodiscard]] const char* ToModuleNodeInfoLanguageKey(ModuleNodeInfoLanguage lang) noexcept;
