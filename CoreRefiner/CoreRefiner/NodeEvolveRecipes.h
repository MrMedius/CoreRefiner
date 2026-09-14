#pragma once

#include "ModuleNodeLabel.h"

#include <cstddef>

namespace NodeEvolveRecipes
{
	struct Recipe
	{
		ModuleNodeLabel a;
		ModuleNodeLabel b;
		ModuleNodeLabel product;
	};

	// 无序一对：存盘时 a 的下标 <= b。不要为反序再写一行。
	inline constexpr Recipe kRecipes[] = {
		{ ModuleNodeLabel::Other_Child, ModuleNodeLabel::Other_Repeat, ModuleNodeLabel::Other_Revive },
	};

	static_assert(ToIndex(kRecipes[0].a) <= ToIndex(kRecipes[0].b));

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

	[[nodiscard]] inline bool TryFind(
		ModuleNodeLabel x,
		ModuleNodeLabel y,
		ModuleNodeLabel& outProduct) noexcept
	{
		ModuleNodeLabel a = x;
		ModuleNodeLabel b = y;
		if (ToIndex(a) > ToIndex(b))
		{
			const ModuleNodeLabel tmp = a;
			a = b;
			b = tmp;
		}
		for (const Recipe& recipe : kRecipes)
		{
			if (recipe.a == a && recipe.b == b)
			{
				outProduct = recipe.product;
				return true;
			}
		}
		return false;
	}
}