#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"

#include "InputCodex.h"
#include "Math.h"

#include "Ball.h"

#include <cstddef>

namespace
{
	constexpr int kBallWarmup = 48;

	bool ScreenToWorldXZ(Graphics& gfx, float sx, float sy, float targetY, XMFLOAT3& outWorld)
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

bool AttackManager::TryGetAimVelocity(XMFLOAT3 playerPos, XMFLOAT3& outVel) const
{
	auto mouse = InputCodex::Get().MousePos();
	XMFLOAT3 worldXZ{};
	if (!ScreenToWorldXZ(gfx, (float)mouse.first, (float)mouse.second, playerPos.y, worldXZ))
	{
		return false;
	}

	XMFLOAT3 dirNorm{ worldXZ.x - playerPos.x, 0.0f, worldXZ.z - playerPos.z };
	Normalize3(dirNorm);
	outVel = { dirNorm.x * 0.05f, 0.0f, dirNorm.z * 0.05f };
	return true;
}

void AttackManager::FireRoots(
	const std::vector<Attack*>& roots,
	XMFLOAT3 pos,
	XMFLOAT3 vel)
{
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

void AttackManager::Update(float dt)
{
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
	playerRemote = 0;
}
