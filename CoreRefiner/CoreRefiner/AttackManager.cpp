#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"

#include "InputCodex.h"
#include "Math.h"

#include "Ball.h"
#include "ModuleDeployer.h"

#include <cstddef>

namespace
{
	constexpr int kBallWarmup = 48;
	constexpr std::size_t kOrbitChildCount = 4;
	constexpr float kOrbitRadius = 2.0f;
	constexpr float kOrbitAngularSpeed = 3.5f;
	constexpr float kOrbitParentLife = 2.0f;
	constexpr DirectX::XMFLOAT3 kChildScale{ 0.35f, 0.35f, 0.35f };

	/**
	 * @brief Append N flat children under subject: Child → AttackBall → Orbit(even phase).
	 */
	void AppendEvenOrbitChildren(
		ModuleRecipe& recipe,
		std::size_t count,
		float radius,
		float angularSpeed,
		DirectX::XMFLOAT3 childScale)
	{
		for (std::size_t i = 0; i < count; ++i)
		{
			const float phase = (count > 0)
				? (DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(count))
				: 0.0f;
			recipe.modules.push_back(ChildRecipeModule::Make());
			recipe.modules.push_back(AttackBallRecipeModule::Make(childScale, true));
			recipe.modules.push_back(OrbitRecipeModule::Make(radius, angularSpeed, phase));
		}
	}

	/** @brief Plain shot: AttackBall only. */
	ModuleRecipe MakeBasicRecipe()
	{
		ModuleRecipe recipe{};
		recipe.modules.push_back(AttackBallRecipeModule::Make());
		return recipe;
	}

	/**
	 * @brief Ctrl: AttackBall + even Child/AttackBall/Orbit×N + Lifetime.
	 */
	ModuleRecipe MakeOrbitFanRecipe()
	{
		ModuleRecipe recipe{};
		recipe.modules.push_back(AttackBallRecipeModule::Make());
		AppendEvenOrbitChildren(
			recipe, kOrbitChildCount, kOrbitRadius, kOrbitAngularSpeed, kChildScale);
		recipe.modules.push_back(LifetimeRecipeModule::Make(kOrbitParentLife));
		return recipe;
	}

	/**
	 * @brief Ctrl+Alt demo multi-shot: AttackBall, AttackBall, Child, Orbit, Lifetime
	 *        → fires bare first ball, then second ball+Lifetime (empty child dropped).
	 */
	ModuleRecipe MakeDualShotRecipe()
	{
		ModuleRecipe recipe{};
		recipe.modules.push_back(AttackBallRecipeModule::Make());
		recipe.modules.push_back(AttackBallRecipeModule::Make());
		recipe.modules.push_back(ChildRecipeModule::Make());
		recipe.modules.push_back(OrbitRecipeModule::Make(kOrbitRadius, kOrbitAngularSpeed, 0.0f));
		recipe.modules.push_back(LifetimeRecipeModule::Make(kOrbitParentLife));
		return recipe;
	}
}

bool ScreenToWorldXZ(Graphics& gfx, float sx, float sy, float targetY,	XMFLOAT3& outWorld)
{
	using namespace DirectX;

	float ndcX = sx * 2.0f / (float)SCREEN_WIDTH - 1.0f;
	float ndcY = 1.0f - sy * 2.0f / (float)SCREEN_HEIGHT;

	XMVECTOR nearNDC = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	XMVECTOR farNDC = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	XMMATRIX viewProj = gfx.GetCamera() * gfx.GetProjection();
	XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

	XMVECTOR nearWorld = XMVector3TransformCoord(nearNDC, invViewProj);
	XMVECTOR farWorld = XMVector3TransformCoord(farNDC, invViewProj);

	XMFLOAT3 rayOrigin, rayDir3;
	XMStoreFloat3(&rayOrigin, nearWorld);
	XMStoreFloat3(&rayDir3, XMVector3Normalize(farWorld - nearWorld));

	if (fabsf(rayDir3.y) < 1e-6f)
		return false;

	float t = (targetY - rayOrigin.y) / rayDir3.y;
	if (t < 0.0f)
		return false;

	outWorld.x = rayOrigin.x + t * rayDir3.x;
	outWorld.y = targetY;
	outWorld.z = rayOrigin.z + t * rayDir3.z;
	return true;
}



AttackManager::AttackManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

	HierarchySpawn::WarmupPool<Ball>(
		attack_Ball, kBallWarmup, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, XMFLOAT3{ 0.0f,0.0f,0.0f });
}

void AttackManager::Update(float dt)
{
	if (pPlayer->GetIsAttack())
	{
		auto pos = pPlayer->GetPosition();
		auto mouse = InputCodex::Get().MousePos();
		XMFLOAT3 worldXZ;

		if (ScreenToWorldXZ(gfx, (float)mouse.first, (float)mouse.second, pos.y, worldXZ))
		{
			XMFLOAT3 dirNorm = { worldXZ.x - pos.x, 0.0f, worldXZ.z - pos.z };
			Normalize3(dirNorm);
			const XMFLOAT3 vel{ dirNorm.x * 0.05f, 0.0f, dirNorm.z * 0.05f };

			ModuleRecipe recipe{};
			if (InputCodex::Get().KeyPressed(VK_CONTROL) && InputCodex::Get().KeyPressed(VK_MENU))
			{
				recipe = MakeDualShotRecipe();
			}
			else if (InputCodex::Get().KeyPressed(VK_CONTROL))
			{
				recipe = MakeOrbitFanRecipe();
			}
			else
			{
				recipe = MakeBasicRecipe();
			}

			const std::vector<Ball*> roots = ModuleDeployer::Deploy(recipe, gfx, rg, pos, pPlayer);
			for (Ball* root : roots)
			{
				if (root == nullptr)
				{
					continue;
				}
				attacks.push_back(root);
				root->SpawnAt(pos, vel);
				playerRemote++;
			}
		}
	}

	for (int i = 0; i < attacks.size(); i++)
	{
		if (attacks[i]->IsActive())
			attacks[i]->Update(dt);
		else
		{
			attacks.erase(attacks.begin() + i);
			i--;
		}
	}
}

void AttackManager::Submit(void)
{
	for (int i = 0; i < attacks.size(); i++)
		if (attacks[i]->IsActive())
			attacks[i]->Submit();
}

void AttackManager::Reset(void)
{
	for (int i = 0; i < attacks.size(); i++)
		if (attacks[i]->IsActive())
			attacks[i]->Deactivate();

	attacks.clear();
}
