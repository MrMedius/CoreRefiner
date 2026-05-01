#include "AttackManager.h"
#include "ObjectCodex.h"

#include "InputCodex.h"
#include "Math.h"

#include "Ball.h"


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

	for (int i = 0;i < 30;i++)
	{
		ObjectCodex::Acquire<Ball>(attack_Ball, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, XMFLOAT3{ 0.0f,0.0f,0.0f });
	}
	for (int i = 0;i < 30;i++)
	{
		ObjectCodex::FindFirstActiveObjectByTag<Ball>(attack_Ball)->Deactivate();
	}
}

void AttackManager::Update(float dt)
{
	// create player's remote attack effect
	if (pPlayer->GetIsAttack())
	{
		auto pos = pPlayer->GetPosition();
		auto mouse = InputCodex::Get().MousePos();
		XMFLOAT3 dir;
		ScreenToWorldXZ(gfx, (float)mouse.first, (float)mouse.second, pos.y, dir);

		XMFLOAT3 dirNorm = { dir.x - pos.x, 0.0f, dir.z - pos.z };
		Normalize3(dirNorm);
	
		attacks.push_back(ObjectCodex::Acquire<Ball>(attack_Ball, gfx, rg, pos, dir)); playerRemote++;
		if (attacks.size() > 0) attacks.back()->SpawnAt(pos, { dirNorm.x * 0.1f, 0.0f, dirNorm.z * 0.1f });
	}

	// update all effects
	for (int i = 0; i < attacks.size(); i++)
	{
		if (attacks[i]->IsActive())
			attacks[i]->Update(dt);
		else
			attacks.erase(attacks.begin() + i);

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