#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"

#include "InputCodex.h"
#include "Math.h"

#include "Ball.h"
#include "SpawnChildModule.h"
#include "LifetimeModule.h"

#include <cstddef>

namespace
{
	/** @brief Shared attack_Ball pool (parents + orbit children). */
	constexpr int kBallWarmup = 48;
	constexpr std::size_t kOrbitChildCount = 4;
	constexpr float kOrbitRadius = 2.0f;
	constexpr float kOrbitAngularSpeed = 3.5f;
	constexpr float kOrbitParentLife = 2.0f;
}

bool ScreenToWorldXZ(Graphics& gfx, float sx, float sy, float targetY,	XMFLOAT3& outWorld)
{
	using namespace DirectX;

	// 1. 屏幕像素 → NDC
	//    正变换: sx = (ndc.x + 1) * 0.5 * W  =>  ndc.x = sx*2/W - 1
	//           sy = (1 - ndc.y) * 0.5 * H  =>  ndc.y = 1 - sy*2/H
	float ndcX = sx * 2.0f / (float)SCREEN_WIDTH - 1.0f;
	float ndcY = 1.0f - sy * 2.0f / (float)SCREEN_HEIGHT;

	// 2. 构造两个 NDC 点（near/far），反投影到世界空间
	//    用 ndc.z=0 和 ndc.z=1 分别代表近/远平面上的点
	XMVECTOR nearNDC = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	XMVECTOR farNDC = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	// 3. 求 ViewProjection 的逆矩阵
	XMMATRIX viewProj = gfx.GetCamera() * gfx.GetProjection();
	XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

	// 4. NDC → 世界空间（XMVector3TransformCoord 会做透视除法）
	XMVECTOR nearWorld = XMVector3TransformCoord(nearNDC, invViewProj);
	XMVECTOR farWorld = XMVector3TransformCoord(farNDC, invViewProj);

	// 5. 构造射线：原点 + 方向
	XMFLOAT3 rayOrigin, rayDir3;
	XMStoreFloat3(&rayOrigin, nearWorld);
	XMStoreFloat3(&rayDir3, XMVector3Normalize(farWorld - nearWorld));

	// 6. 射线与 Y = targetY 平面求交
	//    P(t) = rayOrigin + t * rayDir
	//    P.y = targetY  =>  t = (targetY - rayOrigin.y) / rayDir.y
	if (fabsf(rayDir3.y) < 1e-6f)
		return false;   // 射线平行于水平面，无交点

	float t = (targetY - rayOrigin.y) / rayDir3.y;
	if (t < 0.0f)
		return false;   // 交点在相机后方

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
	// create player's remote attack effect
	if (pPlayer->GetIsAttack())
	{
		auto pos = pPlayer->GetPosition();
		auto mouse = InputCodex::Get().MousePos();
		XMFLOAT3 worldXZ;

		// 获取鼠标指向的世界坐标（在玩家Y高度的平面上）
		if (ScreenToWorldXZ(gfx, (float)mouse.first, (float)mouse.second, pos.y, worldXZ))
		{
			// 计算方向向量（从玩家到鼠标指向点）
			XMFLOAT3 dirNorm = { worldXZ.x - pos.x, 0.0f, worldXZ.z - pos.z };
			Normalize3(dirNorm);
			const XMFLOAT3 vel{ dirNorm.x * 0.05f, 0.0f, dirNorm.z * 0.05f };

			// Ctrl + attack：Ball + 4×SpawnChild + Lifetime（可视化语义：四个子节点）
			if (InputCodex::Get().KeyPressed(VK_CONTROL))
			{
				Ball* parent = ObjectCodex::SpawnPooled<Ball>(attack_Ball, gfx, rg, pos, worldXZ);
				if (parent != nullptr)
				{
					parent->ClearModules();
					for (std::size_t i = 0; i < kOrbitChildCount; ++i)
					{
						const float phase = DirectX::XM_2PI * static_cast<float>(i)
							/ static_cast<float>(kOrbitChildCount);
						parent->AddModule<SpawnChildModule>(
							gfx, rg,
							kOrbitRadius, kOrbitAngularSpeed, phase);
					}
					parent->AddModule<LifetimeModule>(kOrbitParentLife);

					attacks.push_back(parent);
					parent->SpawnAt(pos, vel);
					playerRemote++;
				}
			}
			else
			{
				Ball* pBall = ObjectCodex::SpawnPooled<Ball>(attack_Ball, gfx, rg, pos, worldXZ);
				if (pBall)
				{
					pBall->ClearModules();
					attacks.push_back(pBall);
					pBall->SpawnAt(pos, vel);
					playerRemote++;
				}
			}
		}
	}

	// update all effects
	for (int i = 0; i < attacks.size(); i++)
	{
		if (attacks[i]->IsActive())
			attacks[i]->Update(dt);
		else
		{
			attacks.erase(attacks.begin() + i);
			i--; // 重要：删除后要回退索引，防止跳过下一个元素
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
