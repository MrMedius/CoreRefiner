#include "EnemyManager.h"
#include "ObjectCodex.h"
#include <random>

#include "InputCodex.h"
#include "SoundCodex.h"
#include "GameStatsCodex.h"

#include "Enemy_T.h"

EnemyManager::EnemyManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

	for (int i = 0;i < 10;i++)
	{
		ObjectCodex::Acquire<Enemy_T>(character_Enemy_T, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
	}
	for (int i = 0;i < 10;i++)
	{
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_T>(character_Enemy_T)->Deactivate();
	}
}

void EnemyManager::Update(float dt)
{
	auto position = pPlayer->GetPosition();

	if (CountDown < Interval)
	{
		CountDown += dt;
	}
	if (CountDown >= Interval)
	{
		CountDown = 0.0f;
		NewEnemy = true;
	}
	int type = 0;
	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> d(-30.0f, 30.0f);
	XMFLOAT3 pos = { position.x + d(rng),10.0f,position.z + d(rng) };


	// 新しEnemyTestを生成する
	if (InputCodex::Get().KeyTriggered(KK_NUMPAD1) || type == 1) { enemies.push_back(ObjectCodex::Acquire<Enemy_T>(character_Enemy_T, gfx, rg, pos));	  Created = true; }
	if (Created)
	{
		Created = false;
		enemies.back()->SpawnAt(pos);
		SoundCodex::Get().PlaySE3D(SndPath::SE_Enemy_Create, 0, 1.0f, pos.x, pos.y, pos.z);
	}

	for (int i = 0; i < enemies.size(); i++)
	{
		// 対象かアクティブでいれば更新する、そうでない場合は容器から削除する
		if (enemies[i]->IsActive())
			enemies[i]->Update(dt);
		else
		{
			enemies.erase(enemies.begin() + i);
			--i;
		}
	}
}

void EnemyManager::Submit(void)
{
	for (int i = 0; i < enemies.size(); i++)
		if (enemies[i]->IsActive())
			enemies[i]->Submit();
}

void EnemyManager::Reset(void)
{
	for (int i = 0; i < enemies.size(); i++)
		if (enemies[i]->IsActive())
			enemies[i]->Deactivate();
	enemies.clear();

	NewEnemy = false;
	Created = false;

	isTutorial = true;
}