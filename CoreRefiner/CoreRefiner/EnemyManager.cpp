#include "EnemyManager.h"
#include "ObjectCodex.h"
#include "SoundCodex.h"
#include "Enemy_T.h"
#include "XMath.h"

#include <algorithm>
#include <cmath>

EnemyManager::EnemyManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg),
	rng_(std::random_device{}())
{
	pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
	EnsurePool_(kInitialPool_);
}

void EnemyManager::Update(float dt)
{
	if (pPlayer == nullptr)
	{
		pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
	}

	if (waveActive_ && spawnedThisWave_ < spec_.enemyBudget)
	{
		if (CountDown < Interval)
		{
			CountDown += dt;
		}
		if (CountDown >= Interval)
		{
			CountDown = 0.0f;
			NewEnemy = true;
		}
		if (NewEnemy)
		{
			NewEnemy = false;
			TrySpawnOne_();
		}
	}

	for (int i = 0; i < static_cast<int>(enemies.size()); i++)
	{
		if (enemies[i]->IsActive())
		{
			enemies[i]->Update(dt);
		}
		else
		{
			enemies.erase(enemies.begin() + i);
			--i;
		}
	}
}

void EnemyManager::Submit(void)
{
	for (int i = 0; i < static_cast<int>(enemies.size()); i++)
	{
		if (enemies[i]->IsActive())
		{
			enemies[i]->Submit();
		}
	}
}

void EnemyManager::Reset(void)
{
	ClearAll();
	waveActive_ = false;
	spawnedThisWave_ = 0;
	spec_ = {};
	Interval = 1.5f;
	CountDown = 0.0f;
}

void EnemyManager::SetWave(const WaveSpec& spec)
{
	ClearAll();
	spec_ = spec;
	Interval = (spec_.spawnInterval > 0.0f) ? spec_.spawnInterval : 1.5f;
	CountDown = 0.0f;
	NewEnemy = false;
	spawnedThisWave_ = 0;
	waveActive_ = spec_.enemyBudget > 0;
	EnsurePool_(spec_.enemyBudget);
}

void EnemyManager::ClearAll()
{
	for (Enemy* enemy : enemies)
	{
		if (enemy != nullptr && enemy->IsActive())
		{
			enemy->Deactivate();
		}
	}
	enemies.clear();
	NewEnemy = false;
}

void EnemyManager::HaltSpawning() noexcept
{
	waveActive_ = false;
	NewEnemy = false;
}

void EnemyManager::EnsurePool_(int count)
{
	if (count <= 0)
	{
		return;
	}
	const int have = static_cast<int>(ObjectCodex::FindObjectsByTag<Enemy_T>(character_Enemy_T).size());
	const int need = count - have;
	std::vector<Enemy_T*> batch;
	batch.reserve(static_cast<std::size_t>(std::max(need, 0)));
	for (int i = 0; i < need; ++i)
	{
		if (Enemy_T* enemy = ObjectCodex::SpawnPooled<Enemy_T>(character_Enemy_T, gfx, rg, XMFLOAT3{ 0.0f, 0.0f, 0.0f }))
		{
			batch.push_back(enemy);
		}
	}
	for (Enemy_T* enemy : batch)
	{
		enemy->Deactivate();
	}
}

void EnemyManager::TrySpawnOne_()
{
	if (pPlayer == nullptr || spawnedThisWave_ >= spec_.enemyBudget)
	{
		return;
	}

	const XMFLOAT3 pos = PickSpawnPos_();
	Enemy* enemy = ObjectCodex::SpawnPooled<Enemy_T>(character_Enemy_T, gfx, rg, pos);
	if (enemy == nullptr)
	{
		return;
	}
	enemy->SpawnAt(pos);
	enemies.push_back(enemy);
	++spawnedThisWave_;
	SoundCodex::Get().PlaySE3D(SndPath::SE_Enemy_Create, 0, 1.0f, pos.x, pos.y, pos.z);
}

DirectX::XMFLOAT3 EnemyManager::PickSpawnPos_() noexcept
{
	XMFLOAT3 origin{ 0.0f, 0.0f, 0.0f };
	if (pPlayer != nullptr)
	{
		origin = pPlayer->GetPosition();
	}

	std::uniform_real_distribution<float> angleDist(0.0f, XM_2PI);
	std::uniform_real_distribution<float> unitDist(0.0f, 1.0f);
	const float angle = angleDist(rng_);
	const float minSq = kSpawnMinRadius_ * kSpawnMinRadius_;
	const float maxSq = kSpawnMaxRadius_ * kSpawnMaxRadius_;
	const float radius = std::sqrt(minSq + unitDist(rng_) * (maxSq - minSq));

	XMFLOAT3 pos{
		origin.x + std::cos(angle) * radius,
		kSpawnHeight_,
		origin.z + std::sin(angle) * radius
	};
	pos.x = std::clamp(pos.x, -kFieldHalf_, kFieldHalf_);
	pos.z = std::clamp(pos.z, -kFieldHalf_, kFieldHalf_);
	return pos;
}
