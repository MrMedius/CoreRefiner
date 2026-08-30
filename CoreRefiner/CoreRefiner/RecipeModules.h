#pragma once
#include "RecipeCore.h"

#include <memory>

/**
 * @brief Recipe module: fill the current empty pit with an AttackBall (Ball),
 *        or if focus is an already-filled root, flush that shot and start a new root.
 */
class AttackBallRecipeModule final : public IRecipeModule
{
public:
	/**
	 * @param scale Visual + collider scale for the filled Ball.
	 * @param enableCollider Whether the sphere collider stays enabled.
	 */
	explicit AttackBallRecipeModule(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true) noexcept
		:
		scale_(scale),
		enableCollider_(enableCollider)
	{}

	[[nodiscard]] const char* GetName() const noexcept override { return "AttackBall"; }

	void Apply(BuildContext& ctx) const override
	{
		ctx.EnsureRootStarted();
		if (ctx.focus == nullptr)
		{
			return;
		}

		// Second top-level AttackBall: commit previous root shot, open a new root pit.
		if (ctx.focus == ctx.currentRoot.get() && ctx.focus->IsFilled())
		{
			ctx.FlushCurrentRoot();
			ctx.EnsureRootStarted();
			if (ctx.focus == nullptr)
			{
				return;
			}
		}

		if (ctx.focus->IsFilled())
		{
			// Filled non-root focus (e.g. filled child) — ignore duplicate fill.
			return;
		}

		ctx.focus->entityKind = SlotEntityKind::AttackBall;
		ctx.focus->scale = scale_;
		ctx.focus->enableCollider = enableCollider_;
	}

	/** @brief Factory for recipe lists. */
	static std::unique_ptr<AttackBallRecipeModule> Make(
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f },
		bool enableCollider = true)
	{
		return std::make_unique<AttackBallRecipeModule>(scale, enableCollider);
	}

private:
	DirectX::XMFLOAT3 scale_{ 1.0f, 1.0f, 1.0f };
	bool enableCollider_{ true };
};

/**
 * @brief Recipe module: open an empty child pit under the shot root (扁平挂主体), move focus there.
 * @note Always parents to currentRoot so siblings share even Orbit phases around the subject.
 */
class ChildRecipeModule final : public IRecipeModule
{
public:
	[[nodiscard]] const char* GetName() const noexcept override { return "Child"; }

	void Apply(BuildContext& ctx) const override
	{
		ctx.EnsureRootStarted();
		if (ctx.currentRoot == nullptr || !ctx.currentRoot->IsFilled())
		{
			return;
		}

		auto child = std::make_unique<RecipeSlot>();
		child->parent = ctx.currentRoot.get();
		RecipeSlot* raw = child.get();
		ctx.currentRoot->children.push_back(std::move(child));
		ctx.focus = raw;
	}

	static std::unique_ptr<ChildRecipeModule> Make()
	{
		return std::make_unique<ChildRecipeModule>();
	}
};

/**
 * @brief Recipe module: queue OrbitLocal on the focus pit (may still be empty).
 */
class OrbitRecipeModule final : public IRecipeModule
{
public:
	OrbitRecipeModule(float radius, float phase) noexcept
		:
		radius_(radius),
		phase_(phase)
	{}

	[[nodiscard]] const char* GetName() const noexcept override { return "Orbit"; }

	void Apply(BuildContext& ctx) const override
	{
		if (ctx.focus == nullptr)
		{
			return;
		}
		ctx.focus->pendingOrbits.push_back(PendingOrbit{ radius_, phase_ });
	}

	static std::unique_ptr<OrbitRecipeModule> Make(float radius, float phase)
	{
		return std::make_unique<OrbitRecipeModule>(radius, phase);
	}

private:
	float radius_{ 2.0f };
	float phase_{ 0.0f };
};

/**
 * @brief Recipe module: queue Lifetime on the current shot root (仅主体), independent of focus.
 */
class LifetimeRecipeModule final : public IRecipeModule
{
public:
	explicit LifetimeRecipeModule(float durationSeconds) noexcept
		:
		durationSeconds_(durationSeconds)
	{}

	[[nodiscard]] const char* GetName() const noexcept override { return "Lifetime"; }

	void Apply(BuildContext& ctx) const override
	{
		ctx.EnsureRootStarted();
		if (ctx.currentRoot == nullptr)
		{
			return;
		}
		ctx.currentRoot->pendingLifetimes.push_back(PendingLifetime{ durationSeconds_ });
	}

	static std::unique_ptr<LifetimeRecipeModule> Make(float durationSeconds)
	{
		return std::make_unique<LifetimeRecipeModule>(durationSeconds);
	}

private:
	float durationSeconds_{ 2.0f };
};
