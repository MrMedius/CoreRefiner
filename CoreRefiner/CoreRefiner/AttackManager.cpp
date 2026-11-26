#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"

#include "InputCodex.h"
#include "XMath.h"

#include "Ball.h"
#include "RingYang.h"
#include "RingLesserYang.h"

#include <cmath>
#include <cstddef>

namespace
{
	// 子弹预热个数。
	constexpr int kBallWarmup = 48;
	// 白环预热个数。造好后立刻关掉，不交出，不进连发，也不加仓库伤害。
	constexpr int kRingYangWarmup = 8;
	// 少阳环预热个数。同样先关掉，等爆发时再取出。
	constexpr int kRingLesserYangWarmup = 8;
}

AttackManager::AttackManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	// 先记下玩家。放出时若空或已失效，再查一次。
	pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

	HierarchySpawn::WarmupPool<Ball>(attack_Ball, kBallWarmup, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, XMFLOAT3{ 0.0f,0.0f,0.0f });
	HierarchySpawn::WarmupPool<RingYang>(attack_Ring_Yang, kRingYangWarmup, gfx, rg);
	HierarchySpawn::WarmupPool<RingLesserYang>(attack_Ring_LesserYang, kRingLesserYangWarmup, gfx, rg);
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

void AttackManager::ClaimAttack()
{
	for (AttackBase* atk : ObjectCodex::TakeHandoff<AttackBase>())
	{
		if (atk == nullptr || !atk->IsActive())
		{
			continue;
		}
		bool tracked = false;
		for (AttackBase* existing : attacks)
		{
			if (existing == atk)
			{
				tracked = true;
				break;
			}
		}
		if (!tracked)
		{
			attacks.push_back(atk);
		}
	}
}

void AttackManager::AdoptLive(AttackBase* attack)
{
	if (attack == nullptr)
	{
		return;
	}
	bool tracked = false;
	for (AttackBase* existing : attacks)
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
}

void AttackManager::QueueRoots(const std::vector<AttackBase*>& roots)
{
	// wasEmpty 为真且离上一颗已经够久时，这一批的第一颗当帧就放。
	const bool wasEmpty = pendingShots_.empty();
	for (AttackBase* root : roots)
	{
		if (root == nullptr)
		{
			continue;
		}
		PendingShot shot{};
		shot.root = root;
		CollectAttackTree(root, shot.tree);
		pendingShots_.push_back(std::move(shot));
	}
	if (wasEmpty && sinceLastFire_ >= GameRulesCodex::FireInterval)
	{
		ReleaseNextShot_();
	}
}

void AttackManager::Update(float dt)
{
	// 上一帧交出的攻击在这里进名单。这一帧才卸下或生成的，下一帧再更新。
	ClaimAttack();
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

	// 先更新已经在飞的弹，再放下一颗，避免新弹在出生的同一帧再走一步。
	sinceLastFire_ += dt;
	if (pendingShots_.empty())
	{
		// 空队列封顶在一个间隔，下一颗入队可以当帧放出。
		if (sinceLastFire_ > GameRulesCodex::FireInterval)
		{
			sinceLastFire_ = GameRulesCodex::FireInterval;
		}
	}
	else if (sinceLastFire_ >= GameRulesCodex::FireInterval)
	{
		ReleaseNextShot_();
	}
}

void AttackManager::ReleaseNextShot_()
{
	if (pendingShots_.empty())
	{
		return;
	}

	PendingShot shot = std::move(pendingShots_.front());
	pendingShots_.pop_front();
	// 扣掉一个间隔。多出来的时间留给下一颗，平均间隔不再被帧长拉长。
	sinceLastFire_ -= GameRulesCodex::FireInterval;
	if (sinceLastFire_ < 0.0f)
	{
		sinceLastFire_ = 0.0f;
	}

	AttackBase* root = shot.root;
	if (root == nullptr || !root->IsActive())
	{
		return;
	}

	// 缓存失效时再找。找不到就回收这一颗，不出生。
	if (pPlayer == nullptr || !pPlayer->IsActive())
	{
		pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
	}
	if (pPlayer == nullptr)
	{
		root->Deactivate();
		return;
	}

	const DirectX::XMFLOAT3 pos = pPlayer->GetPosition();
	DirectX::XMFLOAT3 vel{ 0.0f, 0.0f, 0.05f };
	TryGetAimVelocity(pos, vel);
	AdoptLive(root);
	root->SpawnAt(pos, vel);
	// 同一次出生的子弹自己进名单。这一帧的更新已经走过，下一帧才动。
	if (auto* rootMod = dynamic_cast<AttackBase_Moduled*>(root))
	{
		for (AttackBase_Moduled* child : rootMod->TakeBornWith())
		{
			AdoptLive(child);
		}
	}
	if (onLaunched_)
	{
		for (AttackBase* atk : shot.tree)
		{
			if (atk != nullptr && atk->IsActive())
			{
				onLaunched_(*atk);
			}
		}
	}
}

void AttackManager::CollectAttackTree(AttackBase* atk, std::vector<AttackBase*>& out)
{
	if (atk == nullptr)
	{
		return;
	}
	out.push_back(atk);
	// 同伴不是变换子物体。出生前还在这份名单里。
	if (auto* moduled = dynamic_cast<AttackBase_Moduled*>(atk))
	{
		for (AttackBase_Moduled* child : moduled->GetBornWith())
		{
			CollectAttackTree(child, out);
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
	// 还没放出的根弹回收掉，避免波次结束或回备战后继续射。
	for (PendingShot& shot : pendingShots_)
	{
		if (shot.root != nullptr && shot.root->IsActive())
		{
			shot.root->Deactivate();
		}
	}
	pendingShots_.clear();
	sinceLastFire_ = GameRulesCodex::FireInterval;

	for (int i = 0; i < attacks.size(); i++)
		if (attacks[i]->IsActive())
			attacks[i]->Deactivate();

	for (AttackBase* a : ObjectCodex::FindActiveObjectsByTag<AttackBase>(attack_Ball))
	{
		if (a != nullptr && a->IsActive())
		{
			a->Deactivate();
		}
	}

	// 还没认领的攻击关掉，避免波次结束后来才进名单。白环和子弹都在这份名单里。
	for (AttackBase* atk : ObjectCodex::TakeHandoff<AttackBase>())
	{
		if (atk != nullptr && atk->IsActive())
		{
			atk->Deactivate();
		}
	}

	attacks.clear();
}
