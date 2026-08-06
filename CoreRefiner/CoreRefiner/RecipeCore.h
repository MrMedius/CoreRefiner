#pragma once
#include "Ball.h"
#include "Rule_Orbit_Module.h"
#include "Attribute_Lifetime_Module.h"
#include "ObjectCodex.h"
#include "Graphics.h"
#include "RenderGraph.h"

#include <memory>
#include <utility>
#include <vector>

class Player;
struct BuildContext;

/**
 * @brief Entity kind assigned when an AttackBall (or future) recipe module fills a slot.
 */
enum class SlotEntityKind : unsigned char
{
	None,
	AttackBall,
};

/**
 * @brief Orbit queued on a slot before the entity exists (order-independent with AttackBall fill).
 */
struct PendingOrbit
{
	float radius{ 2.0f };
	float angularSpeed{ 3.5f };
	float phase{ 0.0f };
};

/**
 * @brief Lifetime queued on the shot root slot (仅主体).
 */
struct PendingLifetime
{
	float durationSeconds{ 2.0f };
};

/**
 * @brief One pit in the recipe graph: may be empty, filled, and hold pending gameplay modules.
 */
struct RecipeSlot
{
	RecipeSlot* parent{ nullptr };
	std::vector<std::unique_ptr<RecipeSlot>> children;

	SlotEntityKind entityKind{ SlotEntityKind::None };
	Ball* entity{ nullptr };

	DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
	bool enableCollider{ true };

	std::vector<PendingOrbit> pendingOrbits;
	std::vector<PendingLifetime> pendingLifetimes;

	[[nodiscard]] bool IsFilled() const noexcept
	{
		return entityKind != SlotEntityKind::None;
	}
};

/**
 * @brief Mutable builder state while applying recipe modules (配置焦点 + 多发队列).
 */
struct BuildContext
{
	Graphics* gfx{ nullptr };
	Rgph::RenderGraph* rg{ nullptr };
	Player* player{ nullptr };
	DirectX::XMFLOAT3 spawnPos{ 0.0f, 0.0f, 0.0f };
	bool parentRootToPlayer{ false };

	/** @brief Root slot of the shot currently being built (may be empty). */
	std::unique_ptr<RecipeSlot> currentRoot;
	/** @brief Focus pit for the next token (fill / open child / queue module). */
	RecipeSlot* focus{ nullptr };
	/** @brief Materialized roots ready for AttackManager::SpawnAt. */
	std::vector<Ball*> shots;

	/**
	 * @brief Ensure an empty root slot exists and focus it if starting fresh.
	 */
	void EnsureRootStarted()
	{
		if (currentRoot != nullptr)
		{
			return;
		}
		currentRoot = std::make_unique<RecipeSlot>();
		focus = currentRoot.get();
	}

	/**
	 * @brief Recursively spawn filled slots; unfilled children are dropped (no Ball created).
	 * @return Ball for this slot, or nullptr if entityKind is None.
	 */
	Ball* MaterializeSlot(RecipeSlot& slot, RecipeSlot* parentSlot)
	{
		if (!slot.IsFilled() || gfx == nullptr || rg == nullptr)
		{
			// Unfilled pit (e.g. Child + Orbit without AttackBall): discard.
			slot.entity = nullptr;
			return nullptr;
		}

		Ball* ball = ObjectCodex::SpawnPooled<Ball>(
			attack_Ball, *gfx, *rg, spawnPos, DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f });
		if (ball == nullptr)
		{
			return nullptr;
		}

		ball->ClearParent();
		ball->ClearModules();
		ball->SetMoveAccel({ 0.0f, 0.0f, 0.0f });
		ball->ResetMoveVelocity();

		if (parentSlot != nullptr && parentSlot->entity != nullptr)
		{
			ball->SetParent(parentSlot->entity);
		}

		ball->ApplyPresentation(slot.scale, slot.enableCollider);

		for (const PendingOrbit& o : slot.pendingOrbits)
		{
			ball->AddModule<Rule_Orbit_Module>(o.radius, o.angularSpeed, o.phase);
		}
		for (const PendingLifetime& life : slot.pendingLifetimes)
		{
			ball->AddModule<Attribute_Lifetime_Module>(life.durationSeconds);
		}

		slot.entity = ball;

		// Materialize children; unfilled slots return nullptr and leave no entity.
		for (auto& child : slot.children)
		{
			if (child != nullptr)
			{
				(void)MaterializeSlot(*child, &slot);
			}
		}

		return ball;
	}

	/**
	 * @brief Materialize currentRoot into shots and clear in-progress state.
	 */
	void FlushCurrentRoot()
	{
		if (currentRoot == nullptr)
		{
			return;
		}

		Ball* rootBall = MaterializeSlot(*currentRoot, nullptr);
		if (rootBall != nullptr)
		{
			if (parentRootToPlayer && player != nullptr)
			{
				rootBall->SetParent(player);
			}
			shots.push_back(rootBall);
		}

		currentRoot.reset();
		focus = nullptr;
	}
};

/**
 * @brief One recipe token (visual-programming module). Apply mutates BuildContext slots.
 * @note Distinct from runtime IProjectileModule — this is assemble-time only.
 */
class IRecipeModule
{
public:
	virtual ~IRecipeModule() = default;

	/**
	 * @brief Apply this token onto the slot graph / focus / shot queue.
	 */
	virtual void Apply(BuildContext& ctx) const = 0;

	/** @brief Debug / editor name. */
	[[nodiscard]] virtual const char* GetName() const noexcept = 0;
};
