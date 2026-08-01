#pragma once
#include "RecipeCore.h"
#include "RecipeModules.h"

#include <memory>
#include <vector>

/**
 * @brief Ordered list of recipe modules (token stream).
 */
struct ModuleRecipe
{
	std::vector<std::unique_ptr<IRecipeModule>> modules;
	bool parentRootToPlayer{ false };
};

/**
 * @brief Run recipe tokens against a BuildContext, then flush the last in-progress root.
 */
class ModuleDeployer
{
public:
	/**
	 * @brief Apply all recipe modules and return materialized shot roots (may be multiple).
	 * @note Does not call SpawnAt — AttackManager sets velocity and arms modules.
	 */
	static std::vector<Ball*> Deploy(
		const ModuleRecipe& recipe,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 pos,
		Player* player)
	{
		BuildContext ctx{};
		ctx.gfx = &gfx;
		ctx.rg = &rg;
		ctx.player = player;
		ctx.spawnPos = pos;
		ctx.parentRootToPlayer = recipe.parentRootToPlayer;

		for (const auto& module : recipe.modules)
		{
			if (module != nullptr)
			{
				module->Apply(ctx);
			}
		}

		ctx.FlushCurrentRoot();
		return ctx.shots;
	}
};
