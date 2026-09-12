#pragma once
#include "JsonTextCopy.h"
#include "ModuleNodeLabel.h"
#include "TextTypes.h"

#include <filesystem>
#include <string>
#include <vector>

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

bool LoadModuleNodeInfoCopy(const std::filesystem::path& path);

bool LoadModuleNodeInfoCopy();

[[nodiscard]] bool IsModuleNodeInfoCopyLoaded() noexcept;

class IModuleNode;

// 按 GameStatsCodex::GetLanguage() 取词条；缺则回退 En、Zh
[[nodiscard]] const ModuleNodeInfoEntry& GetModuleNodeInfoCopy(ModuleNodeLabel label);

// Fusion 按实例拼：标题「主体 & 素材」，描述先主体后素材。其它 Label 等同查表拷贝。
[[nodiscard]] ModuleNodeInfoEntry ComposeModuleNodeInfoCopy(const IModuleNode& node);

[[nodiscard]] const char* ToModuleNodeLabelName(ModuleNodeLabel label) noexcept;

[[nodiscard]] inline const char* ToModuleNodeInfoLanguageKey(Language lang) noexcept
{
	return ToLanguageKey(lang);
}
