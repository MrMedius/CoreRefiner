#pragma once

#include "ModuleNodeLabel.h"

namespace NodeEvolveRecipes
{
	struct Recipe
	{
		// 主体格里的标签。
		ModuleNodeLabel material;
		// 素材格里的标签。
		ModuleNodeLabel subject;
		// 进化产物。
		ModuleNodeLabel product;
	};

	// 有序表。主体在前，素材在后。放反要另写一行。
	inline constexpr Recipe kRecipes[] = {
		{ ModuleNodeLabel::Rule_Child,		ModuleNodeLabel::Other_Repeat,	ModuleNodeLabel::Rule_Revive },
		{ ModuleNodeLabel::Other_Repeat,	ModuleNodeLabel::Rule_Child,	ModuleNodeLabel::Rule_Revive },
		{ ModuleNodeLabel::Element_Yin,		ModuleNodeLabel::Element_Yin,	ModuleNodeLabel::Element_GreaterYin },
		{ ModuleNodeLabel::Element_Yang,	ModuleNodeLabel::Element_Yin,	ModuleNodeLabel::Element_LesserYang },
		{ ModuleNodeLabel::Element_Yin,		ModuleNodeLabel::Element_Yang,	ModuleNodeLabel::Element_LesserYin },
		{ ModuleNodeLabel::Element_Yang,	ModuleNodeLabel::Element_Yang,	ModuleNodeLabel::Element_GreaterYang },
	};

	// 检测产物必须落在 ModuleNodeLabel.h 的进化表里。本文件不持有那张表。
	[[nodiscard]] inline constexpr bool AllProductsInEvolveLabelTable() noexcept
	{
		for (const Recipe& recipe : kRecipes)
		{
			if (!LabelTableContains(kEvolveLabels, recipe.product))
			{
				return false;
			}	
		}
		return true;
	}
	static_assert(AllProductsInEvolveLabelTable());

	// 比较一条配方。subject 是主体标签，material 是素材标签。两边相同也算对上。
	[[nodiscard]] inline constexpr bool PairMatches(
		const Recipe& recipe,
		ModuleNodeLabel subject,
		ModuleNodeLabel material) noexcept
	{
		return recipe.subject == subject && recipe.material == material;
	}

	// 不放进 kRecipes。只确认主体和素材是同一个标签时，比较仍然成立。
	static_assert(PairMatches(
		Recipe{ ModuleNodeLabel::Rule_Child, ModuleNodeLabel::Rule_Child, ModuleNodeLabel::Rule_Child },
		ModuleNodeLabel::Rule_Child,
		ModuleNodeLabel::Rule_Child));

	// 按主体、素材原样查找。subject 是主体标签，material 是素材标签，outProduct 写出产物。对上返回 true。
	[[nodiscard]] inline bool TryFind(
		ModuleNodeLabel subject,
		ModuleNodeLabel material,
		ModuleNodeLabel& outProduct) noexcept
	{
		for (const Recipe& recipe : kRecipes)
		{
			if (PairMatches(recipe, subject, material))
			{
				outProduct = recipe.product;
				return true;
			}
		}
		return false;
	}
}
