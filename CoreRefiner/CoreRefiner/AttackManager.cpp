#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"

#include "InputCodex.h"
#include "Math.h"

#include "Ball.h"
#include "AttackDeployer.h"

#include <cstddef>

namespace
{
	constexpr int kBallWarmup = 48;
	constexpr std::size_t kOrbitChildCount = 4;
	constexpr float kOrbitRadius = 2.0f;
	constexpr float kOrbitAngularSpeed = 3.5f;
	constexpr DirectX::XMFLOAT3 kChildScale{ 0.5f, 0.5f, 0.5f };

	// Case1: { Spawn_Ball }
	AttackRecipe MakeBasicRecipe()
	{
		AttackRecipe recipe{};
		recipe.steps.push_back(DeployStep_Spawn_Ball::Make());
		return recipe;
	}

	// Case2: subject + 4 orbiting children.
	// Token stream is Child→Spawn_Ball→Orbit ×4 (Child alone does not create a Ball).
	AttackRecipe MakeOrbitFanRecipe()
	{
		AttackRecipe recipe{};
		recipe.steps.push_back(DeployStep_Spawn_Ball::Make());
		for (std::size_t i = 0; i < kOrbitChildCount; ++i)
		{
			recipe.steps.push_back(DeployStep_Other_Child::Make());
			recipe.steps.push_back(DeployStep_Spawn_Ball::Make(kChildScale, true));
			recipe.steps.push_back(DeployStep_Rule_Orbit::Make(kOrbitRadius, kOrbitAngularSpeed));
		}
		return recipe;
	}

	// Case3: fire bare ball, then a second ball with one non-orbiting child.
	// Second Spawn_Ball flushes the first shot; Child→Spawn fills the child pit.
	AttackRecipe MakeDualChildRecipe()
	{
		AttackRecipe recipe{};
		recipe.steps.push_back(DeployStep_Spawn_Ball::Make());
		recipe.steps.push_back(DeployStep_Rule_Orbit::Make(2.0f, 1.0f));

		recipe.steps.push_back(DeployStep_Spawn_Ball::Make());
		recipe.steps.push_back(DeployStep_Other_Child::Make());
		recipe.steps.push_back(DeployStep_Spawn_Ball::Make(kChildScale, true));
		recipe.steps.push_back(DeployStep_Attribute_SpeedRate::Make(0.7f));
		recipe.steps.push_back(DeployStep_Rule_Orbit::Make(2.0f, 1.0f));

		recipe.steps.push_back(DeployStep_Spawn_Ball::Make(kChildScale, true));
		recipe.steps.push_back(DeployStep_Attribute_SpeedRate::Make(0.4f));

		recipe.steps.push_back(DeployStep_Spawn_Ball::Make());
		recipe.steps.push_back(DeployStep_Other_Child::Make());
		recipe.steps.push_back(DeployStep_Spawn_Ball::Make(kChildScale, true));
		recipe.steps.push_back(DeployStep_Attribute_SpeedRate::Make(0.1f));

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

			AttackRecipe recipe{};
			if (InputCodex::Get().KeyPressed(VK_CONTROL) && InputCodex::Get().KeyPressed(VK_MENU))
			{
				recipe = MakeDualChildRecipe();
			}
			else if (InputCodex::Get().KeyPressed(VK_CONTROL))
			{
				recipe = MakeOrbitFanRecipe();
			}
			else
			{
				recipe = MakeBasicRecipe();
			}

			const std::vector<Attack*> roots = AttackDeployer::Deploy(recipe, gfx, rg, pos, pPlayer);
			for (Attack* root : roots)
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
