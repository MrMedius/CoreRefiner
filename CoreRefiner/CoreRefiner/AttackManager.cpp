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
	// 先记下玩家。放出时若空或已失效，再查一次。
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

void AttackManager::QueueRoots(const std::vector<Attack*>& roots)
{
	// wasEmpty 为真且离上一颗已经够久时，这一批的第一颗当帧就放。
	const bool wasEmpty = pendingShots_.empty();
	for (Attack* root : roots)
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

	Attack* root = shot.root;
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
	if (onLaunched_)
	{
		for (Attack* atk : shot.tree)
		{
			if (atk != nullptr && atk->IsActive())
			{
				onLaunched_(*atk);
			}
		}
	}
}

void AttackManager::CollectAttackTree(Attack* atk, std::vector<Attack*>& out)
{
	if (atk == nullptr)
	{
		return;
	}
	out.push_back(atk);
	for (std::size_t i = 0; i < atk->GetChildCount(); ++i)
	{
		CollectAttackTree(dynamic_cast<Attack*>(atk->GetChild(i)), out);
	}
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

	for (Attack* a : ObjectCodex::FindActiveObjectsByTag<Attack>(attack_Ball))
	{
		if (a != nullptr && a->IsActive())
		{
			a->Deactivate();
		}
	}

	attacks.clear();
}
