#pragma once
#include "IProjectileModule.h"
#include "Attack.h"
#include "Ball.h"
#include "OrbitLocalModule.h"
#include "ObjectCodex.h"
#include "Graphics.h"
#include "RenderGraph.h"

/**
 * @brief Spawn module: one instance spawns exactly one child Ball (attack_Ball pool) and parents it.
 * @note Visual-programming semantics — N SpawnChild modules ⇒ N children after OnSpawn chain.
 *       Child receives OrbitLocalModule; ApplyPresentation keeps scale and sphere radius matched.
 */
class SpawnChildModule : public IProjectileModule
{
public:
	/**
	 * @param owner Parent Attack (injected by AddModule).
	 * @param gfx / rg Forwarded to child SpawnPooled&lt;Ball&gt;.
	 * @param orbitRadius / orbitAngularSpeed / orbitPhase Params for child's OrbitLocalModule.
	 * @param childScale Visual + collider scale (radius = max component; unit Ball uses 1).
	 * @param enableChildCollider Whether the child sphere collider stays enabled.
	 */
	SpawnChildModule(
		Attack* owner,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		float orbitRadius,
		float orbitAngularSpeed,
		float orbitPhase,
		XMFLOAT3 childScale = { 0.35f, 0.35f, 0.35f },
		bool enableChildCollider = true) noexcept
		:
		IProjectileModule(owner),
		gfx_(gfx),
		rg_(rg),
		orbitRadius_(orbitRadius),
		orbitAngularSpeed_(orbitAngularSpeed),
		orbitPhase_(orbitPhase),
		childScale_(childScale),
		enableChildCollider_(enableChildCollider)
	{}

	void OnSpawn() override
	{
		Attack* parent = GetOwner();
		if (parent == nullptr)
		{
			return;
		}

		Ball* child = ObjectCodex::SpawnPooled<Ball>(
			attack_Ball, gfx_, rg_,
			XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 0.0f });
		if (child == nullptr)
		{
			return;
		}

		child->ClearParent();
		child->SetParent(parent);

		child->ClearModules();
		child->AddModule<OrbitLocalModule>(orbitRadius_, orbitAngularSpeed_, orbitPhase_);

		// SpawnAt restores unit presentation then runs OrbitLocal OnSpawn.
		child->SpawnAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
		// Override to child scale; keep collider radius matched to visual scale.
		child->ApplyPresentation(childScale_, enableChildCollider_);

		spawnedChild_ = child;
	}

	void OnRecycle() override
	{
		spawnedChild_ = nullptr;
	}

	/** @brief Last child spawned by this module instance (non-owning; may be inactive). */
	[[nodiscard]] Ball* GetSpawnedChild() const noexcept { return spawnedChild_; }

private:
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;
	float orbitRadius_{ 2.0f };
	float orbitAngularSpeed_{ 3.5f };
	float orbitPhase_{ 0.0f };
	XMFLOAT3 childScale_{ 0.35f, 0.35f, 0.35f };
	bool enableChildCollider_{ true };
	Ball* spawnedChild_{ nullptr };
};
