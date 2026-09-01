#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"

#include "InputCodex.h"
#include "XMath.h"

#include "Ball.h"

#include <cmath>
#include <cstddef>

namespace
{
	constexpr int kBallWarmup = 48;
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

bool AttackManager::TryScreenToWorldXZ(
	Graphics& gfx,
	float sx,
	float sy,
	float targetY,
	XMFLOAT3& outWorld)
{
	using namespace DirectX;

	const float ndcX = sx * 2.0f / static_cast<float>(SCREEN_WIDTH) - 1.0f;
	const float ndcY = 1.0f - sy * 2.0f / static_cast<float>(SCREEN_HEIGHT);

	const XMVECTOR nearNDC = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	const XMVECTOR farNDC = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	const XMMATRIX viewProj = gfx.GetCamera() * gfx.GetProjection();
	const XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

	const XMVECTOR nearWorld = XMVector3TransformCoord(nearNDC, invViewProj);
	const XMVECTOR farWorld = XMVector3TransformCoord(farNDC, invViewProj);

	XMFLOAT3 rayOrigin{};
	XMFLOAT3 rayDir3{};
	XMStoreFloat3(&rayOrigin, nearWorld);
	XMStoreFloat3(&rayDir3, XMVector3Normalize(farWorld - nearWorld));

	if (fabsf(rayDir3.y) < 1e-6f)
	{
		return false;
	}

	const float t = (targetY - rayOrigin.y) / rayDir3.y;
	if (t < 0.0f)
	{
		return false;
	}

	outWorld.x = rayOrigin.x + t * rayDir3.x;
	outWorld.y = targetY;
	outWorld.z = rayOrigin.z + t * rayDir3.z;
	return true;
}

bool AttackManager::TryGetAimVelocity(XMFLOAT3 playerPos, XMFLOAT3& outVel) const
{
	auto mouse = InputCodex::Get().MousePos();
	XMFLOAT3 worldXZ{};
	if (!TryScreenToWorldXZ(gfx, (float)mouse.first, (float)mouse.second, playerPos.y, worldXZ))
	{
		return false;
	}

	outVel = ((V(worldXZ) - V(playerPos)).NormalizedXZ() * kAimSpeed).ToFloat3();
	return true;
}

void AttackManager::AdoptLive(Attack* attack)
{
	if (attack == nullptr)
	{
		return;
	}
	bool tracked = false;
	for (Attack* existing : attacks)
	{
		if (existing == attack)
		{
			tracked = true;
			break;
		}
	}
	if (!tracked)
	{
		attacks.push_back(attack);
	}
	attack->SetAdoptLive([this](Attack* live) { AdoptLive(live); });
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
		AdoptLive(root);
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
	AdoptUnparentedAttacks_();
}

void AttackManager::AdoptUnparentedAttacks_()
{
	const std::vector<Attack*> live = ObjectCodex::FindActiveObjectsByTag<Attack>(attack_Ball);
	for (Attack* a : live)
	{
		if (a == nullptr || !a->IsActive() || !a->IsAwaitingManagerAdopt())
		{
			continue;
		}
		a->SetAwaitingManagerAdopt(false);
		AdoptLive(a);
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

	for (Attack* a : ObjectCodex::FindActiveObjectsByTag<Attack>(attack_Ball))
	{
		if (a != nullptr && a->IsActive())
		{
			a->Deactivate();
		}
	}

	attacks.clear();
	playerRemote = 0;
}
