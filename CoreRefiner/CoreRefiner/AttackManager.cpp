#include "AttackManager.h"
#include "ObjectCodex.h"
#include "HierarchySpawn.h"
#include "ColliderComponentBase.h"

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
	XMFLOAT3 vel,
	const std::function<void(Attack*)>& onFired)
{
	// 放出时改读当下的位置和准星，这里的出生点不再使用。
	(void)pos;
	(void)vel;

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
		shot.onFired = onFired;
		MuteColliders_(root, shot.colliderRestore);
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
	if (sinceLastFire_ >= GameRulesCodex::FireInterval)
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
	sinceLastFire_ = 0.0f;

	Attack* root = shot.root;
	if (root == nullptr || !root->IsActive())
	{
		return;
	}

	// player 是这一刻还在场上的玩家。没有就回收，不出生。
	Player* player = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
	if (player == nullptr)
	{
		root->Deactivate();
		return;
	}

	const DirectX::XMFLOAT3 pos = player->GetPosition();
	DirectX::XMFLOAT3 vel{ 0.0f, 0.0f, 0.05f };
	TryGetAimVelocity(pos, vel);
	AdoptLive(root);
	root->SpawnAt(pos, vel);
	// 排队时关过的碰撞重新打开。出生流程会按关掉的状态再写一次，所以放在 SpawnAt 之后。
	for (Attack* atk : shot.colliderRestore)
	{
		if (atk == nullptr || !atk->IsActive())
		{
			continue;
		}
		if (ColliderComponentBase* col = atk->GetComponent<ColliderComponentBase>())
		{
			col->SetEnabled(true);
			col->SyncFromOwner();
		}
	}
	if (shot.onFired)
	{
		shot.onFired(root);
	}
	playerRemote++;
}

void AttackManager::MuteColliders_(Attack* attack, std::vector<Attack*>& restore)
{
	if (attack == nullptr)
	{
		return;
	}
	if (ColliderComponentBase* col = attack->GetComponent<ColliderComponentBase>())
	{
		if (col->IsEnabled())
		{
			col->SetEnabled(false);
			restore.push_back(attack);
		}
	}
	for (std::size_t i = 0; i < attack->GetChildCount(); ++i)
	{
		MuteColliders_(dynamic_cast<Attack*>(attack->GetChild(i)), restore);
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
	playerRemote = 0;
}
