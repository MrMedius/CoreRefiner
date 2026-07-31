#pragma once
#include "Ball.h"
#include "SpawnChildModule.h"
#include "OrbitLocalModule.h"
#include "LifetimeModule.h"
#include "ObjectCodex.h"
#include "Graphics.h"
#include "RenderGraph.h"

#include <cstddef>
#include <vector>

class Player;

/**
 * @brief Who a ModuleStepKind is allowed to configure.
 */
enum class ModuleTargetScope : unsigned char
{
	/** @brief Apply to the current config target (root or child). */
	All,
	/** @brief Always configure the recipe root (主体). */
	RootOnly,
	/** @brief Only configure when current is a child (仅子体). */
	ChildOnly,
};

/**
 * @brief Recipe atom kinds (visual-programming nodes).
 */
enum class ModuleStepKind : unsigned char
{
	Child,
	Orbit,
	Lifetime,
};

/**
 * @brief Per-step parameters (fields unused by a kind are ignored).
 */
struct ModuleStep
{
	ModuleStepKind kind{ ModuleStepKind::Lifetime };

	float orbitRadius{ 2.0f };
	float orbitAngularSpeed{ 3.5f };
	float orbitPhase{ 0.0f };

	DirectX::XMFLOAT3 childScale{ 0.35f, 0.35f, 0.35f };
	bool childEnableCollider{ true };

	float lifetimeSeconds{ 2.0f };
};

/**
 * @brief Ordered list of step kinds (+ params) fed into ModuleDeployer.
 */
struct ModuleRecipe
{
	std::vector<ModuleStep> steps;
	bool parentRootToPlayer{ false };
};

/**
 * @brief Mutable state while walking a recipe (配置对象 cursor).
 */
struct ModuleDeployContext
{
	Ball* root{ nullptr };
	/** @brief Current configuration target (starts as root). */
	Attack* current{ nullptr };
	Graphics* gfx{ nullptr };
	Rgph::RenderGraph* rg{ nullptr };
};

/**
 * @brief Static metadata + execute event for one ModuleStepKind.
 */
struct ModuleKindDesc
{
	ModuleStepKind kind{};
	/** @brief Whether this step attaches an IProjectileModule. */
	bool hasModule{ false };
	/** @brief Human-readable module type name (documentation / debug). */
	const char* moduleTypeName{ nullptr };
	ModuleTargetScope targetScope{ ModuleTargetScope::All };
	/**
	 * @brief Execute event: attach modules / move cursor (replaces Deploy switch).
	 */
	void (*execute)(ModuleDeployContext& ctx, const ModuleStep& step);
};

namespace ModuleStepFactory
{
	inline ModuleStep Child(
		DirectX::XMFLOAT3 scale = { 0.35f, 0.35f, 0.35f },
		bool enableCollider = true) noexcept
	{
		ModuleStep s{};
		s.kind = ModuleStepKind::Child;
		s.childScale = scale;
		s.childEnableCollider = enableCollider;
		return s;
	}

	inline ModuleStep Orbit(float radius, float angularSpeed, float phase) noexcept
	{
		ModuleStep s{};
		s.kind = ModuleStepKind::Orbit;
		s.orbitRadius = radius;
		s.orbitAngularSpeed = angularSpeed;
		s.orbitPhase = phase;
		return s;
	}

	inline ModuleStep Lifetime(float durationSeconds) noexcept
	{
		ModuleStep s{};
		s.kind = ModuleStepKind::Lifetime;
		s.lifetimeSeconds = durationSeconds;
		return s;
	}
}

namespace ModuleKindExec
{
	/**
	 * @brief Resolve attach host from scope + cursor; nullptr = skip.
	 */
	inline Attack* ResolveHost(const ModuleDeployContext& ctx, ModuleTargetScope scope) noexcept
	{
		if (ctx.root == nullptr || ctx.current == nullptr)
		{
			return nullptr;
		}
		switch (scope)
		{
		case ModuleTargetScope::All:
			return ctx.current;
		case ModuleTargetScope::RootOnly:
			return ctx.root;
		case ModuleTargetScope::ChildOnly:
			return (ctx.current != ctx.root) ? ctx.current : nullptr;
		}
		return nullptr;
	}

	/** @brief Child: SpawnChildModule on root → cursor = new child. */
	inline void ExecChild(ModuleDeployContext& ctx, const ModuleStep& step)
	{
		Attack* host = ResolveHost(ctx, ModuleTargetScope::RootOnly);
		if (host == nullptr || ctx.gfx == nullptr || ctx.rg == nullptr)
		{
			return;
		}

		auto* mod = host->AddModule<SpawnChildModule>(
			*ctx.gfx, *ctx.rg, step.childScale, step.childEnableCollider);
		if (mod == nullptr)
		{
			return;
		}

		Ball* child = mod->EnsureSpawned();
		if (child != nullptr)
		{
			ctx.current = child;
		}
	}

	/** @brief Orbit: OrbitLocalModule on current child only. */
	inline void ExecOrbit(ModuleDeployContext& ctx, const ModuleStep& step)
	{
		Attack* host = ResolveHost(ctx, ModuleTargetScope::ChildOnly);
		if (host == nullptr)
		{
			return;
		}
		host->AddModule<OrbitLocalModule>(
			step.orbitRadius, step.orbitAngularSpeed, step.orbitPhase);
	}

	/** @brief Lifetime: LifetimeModule on root (仅主体). */
	inline void ExecLifetime(ModuleDeployContext& ctx, const ModuleStep& step)
	{
		Attack* host = ResolveHost(ctx, ModuleTargetScope::RootOnly);
		if (host == nullptr)
		{
			return;
		}
		host->AddModule<LifetimeModule>(step.lifetimeSeconds);
	}
}

/**
 * @brief Kind registry: 含模组 / 模组名 / 作用对象 / 执行事件.
 */
inline const ModuleKindDesc* FindModuleKindDesc(ModuleStepKind kind) noexcept
{
	static const ModuleKindDesc kTable[] = {
		{
			ModuleStepKind::Child,
			true,
			"SpawnChildModule",
			ModuleTargetScope::RootOnly,
			&ModuleKindExec::ExecChild
		},
		{
			ModuleStepKind::Orbit,
			true,
			"OrbitLocalModule",
			ModuleTargetScope::ChildOnly,
			&ModuleKindExec::ExecOrbit
		},
		{
			ModuleStepKind::Lifetime,
			true,
			"LifetimeModule",
			ModuleTargetScope::RootOnly,
			&ModuleKindExec::ExecLifetime
		},
	};

	for (const ModuleKindDesc& d : kTable)
	{
		if (d.kind == kind)
		{
			return &d;
		}
	}
	return nullptr;
}

/**
 * @brief Walk recipe steps via KindDesc.execute (no Deploy-side switch on behavior).
 */
class ModuleDeployer
{
public:
	/**
	 * @brief Spawn/reuse root, run recipe execute events, return root (SpawnAt later).
	 */
	static Ball* Deploy(
		const ModuleRecipe& recipe,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 pos,
		Player* player)
	{
		Ball* root = ObjectCodex::SpawnPooled<Ball>(
			attack_Ball, gfx, rg, pos, DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f });
		if (root == nullptr)
		{
			return nullptr;
		}

		root->ClearParent();
		root->ClearModules();

		if (recipe.parentRootToPlayer)
		{
			if (player == nullptr)
			{
				return nullptr;
			}
			root->SetParent(player);
		}

		ModuleDeployContext ctx{};
		ctx.root = root;
		ctx.current = root;
		ctx.gfx = &gfx;
		ctx.rg = &rg;

		for (const ModuleStep& step : recipe.steps)
		{
			const ModuleKindDesc* desc = FindModuleKindDesc(step.kind);
			if (desc == nullptr || desc->execute == nullptr)
			{
				continue;
			}
			desc->execute(ctx, step);
		}

		return root;
	}
};
