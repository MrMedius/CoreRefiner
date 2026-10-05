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
	// Fusion Compose 分块：主体、素材各一段。叶子为空，仍读 body。
	std::vector<std::string> bodies;
	std::vector<Text::Span> spans;

	[[nodiscard]] std::vector<std::string> BodyBlocks() const
	{
		if (!bodies.empty())
		{
			return bodies;
		}
		if (body.empty())
		{
			return {};
		}
		return { body };
	}

	[[nodiscard]] std::string JoinedBody() const
	{
		if (bodies.empty())
		{
			return body;
		}
		std::string joined;
		for (const std::string& block : bodies)
		{
			if (block.empty())
			{
				continue;
			}
			if (!joined.empty())
			{
				joined += "\n";
			}
			joined += block;
		}
		return joined;
	}

	[[nodiscard]] std::string ComposedText() const
	{
		const std::string joined = JoinedBody();
		if (title.empty())
		{
			return joined;
		}
		if (joined.empty())
		{
			return title;
		}
		return title + "\n" + joined;
	}
};

// 说明书 JSON stats 键：四基础 + Attribute/Passive 独有 + Repeat 独有
enum class ModuleNodeStat : unsigned char
{
	NodeRadius,
	Cooldown,
	ExpandRadius,
	ExpandSpeed,
	LifetimeRate,
	SpeedRate,
	SizeRate,
	DamageRate,
	DamageFix,
	RepeatCount,
	Count
};

bool LoadModuleNodeInfoCopy(const std::filesystem::path& path);

bool LoadModuleNodeInfoCopy();

[[nodiscard]] bool IsModuleNodeInfoCopyLoaded() noexcept;

class IModuleNode;

// 按 GameStatsCodex::GetLanguage() 取词条；缺则回退 En、Zh
[[nodiscard]] const ModuleNodeInfoEntry& GetModuleNodeInfoCopy(ModuleNodeLabel label);

// Kind 显示名。缺词条时回退 En、Zh，再没有则返回枚举名
[[nodiscard]] std::string GetKindCopy(ModuleNodeKind kind);

// 属性显示名。缺词条时回退 En、Zh，再没有则返回 JSON 键
[[nodiscard]] std::string GetStatCopy(ModuleNodeStat stat);

// Fusion 按实例拼：标题「主体 & 素材」，描述分主体/素材两块。
// 奥义标题用节点上的名字，正文为空，不跟语言表。其它 Label 等同查表拷贝。
[[nodiscard]] ModuleNodeInfoEntry ComposeModuleNodeInfoCopy(const IModuleNode& node);

[[nodiscard]] const char* ToModuleNodeLabelName(ModuleNodeLabel label) noexcept;

[[nodiscard]] inline const char* ToModuleNodeInfoLanguageKey(Language lang) noexcept
{
	return ToLanguageKey(lang);
}
