#pragma once
#include "IProjectileModule.h"
#include "Attack.h"
#include "Ball.h"
#include "ObjectCodex.h"
#include "Graphics.h"
#include "RenderGraph.h"

/**
 * @brief Spawn module on the root: creates exactly one child Ball (no orbit).
 * @note Orbit is a separate recipe step attached to the child by ModuleDeployer.
 */
class SpawnChildModule : public IProjectileModule
{
public:
	/**
	 * @param owner Parent Attack (recipe root; injected by AddModule).
	 * @param gfx / rg Forwarded to child SpawnPooled&lt;Ball&gt;.
	 * @param childScale Visual + collider scale after spawn.
	 * @param enableChildCollider Whether the child sphere collider stays enabled.
	 */
	SpawnChildModule(
		Attack* owner,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		XMFLOAT3 childScale = { 0.35f, 0.35f, 0.35f },
		bool enableChildCollider = true) noexcept
		:
		IProjectileModule(owner),
		gfx_(gfx),
		rg_(rg),
		childScale_(childScale),
		enableChildCollider_(enableChildCollider)
	{}

	/**
	 * @brief Create the child immediately (Deploy-time cursor needs the instance).
	 * @return Non-owning child, or nullptr on pool failure.
	 * @note Idempotent: returns existing spawnedChild_ if still active.
	 */
	Ball* EnsureSpawned()
	{
		if (spawnedChild_ != nullptr && spawnedChild_->IsActive())
		{
			spawnedChild_->ApplyPresentation(childScale_, enableChildCollider_);
			return spawnedChild_;
		}

		Attack* parent = GetOwner();
		if (parent == nullptr)
		{
			return nullptr;
		}

		Ball* child = ObjectCodex::SpawnPooled<Ball>(
			attack_Ball, gfx_, rg_,
			XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 0.0f });
		if (child == nullptr)
		{
			return nullptr;
		}

		child->ClearParent();
		child->SetParent(parent);
		child->ClearModules();
		child->SetMoveAccel({ 0.0f, 0.0f, 0.0f });
		child->ResetMoveVelocity();
		child->ApplyPresentation(childScale_, enableChildCollider_);
		spawnedChild_ = child;
		return spawnedChild_;
	}

	void OnSpawn() override
	{
		EnsureSpawned();
	}

	void OnRecycle() override
	{
		spawnedChild_ = nullptr;
	}

	[[nodiscard]] Ball* GetSpawnedChild() const noexcept { return spawnedChild_; }

private:
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	XMFLOAT3 childScale_{ 0.35f, 0.35f, 0.35f };
	bool enableChildCollider_{ true };
	Ball* spawnedChild_{ nullptr };
};
