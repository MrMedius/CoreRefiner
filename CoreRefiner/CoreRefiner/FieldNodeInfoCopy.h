#pragma once
#include "AttackNodeLabel.h"
#include "TextTypes.h"

#include <filesystem>
#include <string>
#include <vector>


enum class FieldNodeInfoLanguage : unsigned char
{
	Zh,
	Ja,
	En,
};

struct FieldNodeInfoEntry
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

void SetFieldNodeInfoLanguage(FieldNodeInfoLanguage lang) noexcept;

[[nodiscard]] FieldNodeInfoLanguage GetFieldNodeInfoLanguage() noexcept;

bool LoadFieldNodeInfoCopy(const std::filesystem::path& path);

[[nodiscard]] bool IsFieldNodeInfoCopyLoaded() noexcept;

[[nodiscard]] const FieldNodeInfoEntry& GetFieldNodeInfoCopy(AttackNodeLabel label);

[[nodiscard]] const char* ToAttackNodeLabelName(AttackNodeLabel label) noexcept;

[[nodiscard]] const char* ToFieldNodeInfoLanguageKey(FieldNodeInfoLanguage lang) noexcept;
