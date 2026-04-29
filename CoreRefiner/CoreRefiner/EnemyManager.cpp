#include "EnemyManager.h"
#include "ObjectCodex.h"
#include <random>

#include "InputCodex.h"
#include "SoundCodex.h"
#include "GameStatsCodex.h"

#include "Enemy_Red_T.h"
#include "Enemy_Red_0.h"
#include "Enemy_Red_1.h"
#include "Enemy_Green_T.h"
#include "Enemy_Green_0.h"
#include "Enemy_Green_1.h"
#include "Enemy_Blue_T.h"
#include "Enemy_Blue_0.h"
#include "Enemy_Blue_1.h"
//#include "EnemyBoss.h"

EnemyManager::EnemyManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

	auto mapSize = ObjectCodex::FindFirstObjectByTag<ObjectBase>(environment_BlockTiled)->GetSize();
	MapEdgeLeftAndRight = mapSize.x / 2.0f;
	MapEdgeFrontAndBack = mapSize.z / 2.0f;
	for (int i = 0;i < 10;i++)
	{
		ObjectCodex::Acquire<Enemy_Red_0>(character_Enemy_Red_0, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
		ObjectCodex::Acquire<Enemy_Red_1>(character_Enemy_Red_1, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
		ObjectCodex::Acquire<Enemy_Green_0>(character_Enemy_Green_0, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
		ObjectCodex::Acquire<Enemy_Green_1>(character_Enemy_Green_1, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
		ObjectCodex::Acquire<Enemy_Blue_0>(character_Enemy_Blue_0, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
		ObjectCodex::Acquire<Enemy_Blue_1>(character_Enemy_Blue_1, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
	}
	for (int i = 0;i < 10;i++)
	{
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_Red_0>(character_Enemy_Red_0)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_Red_1>(character_Enemy_Red_1)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_Green_0>(character_Enemy_Green_0)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_Green_1>(character_Enemy_Green_1)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_Blue_0>(character_Enemy_Blue_0)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Enemy_Blue_1>(character_Enemy_Blue_1)->Deactivate();
	}
	ObjectCodex::Acquire<Enemy_Red_T>(character_Enemy_Red_T, gfx, rg);
	ObjectCodex::Acquire<Enemy_Green_T>(character_Enemy_Green_T, gfx, rg);
	ObjectCodex::Acquire<Enemy_Blue_T>(character_Enemy_Blue_T, gfx, rg);
	ObjectCodex::FindFirstActiveObjectByTag<Enemy_Red_T>(character_Enemy_Red_T)->Deactivate();
	ObjectCodex::FindFirstActiveObjectByTag<Enemy_Green_T>(character_Enemy_Green_T)->Deactivate();
	ObjectCodex::FindFirstActiveObjectByTag<Enemy_Blue_T>(character_Enemy_Blue_T)->Deactivate();
}

void EnemyManager::Update(float dt, bool isInCD)
{
	if (isTutorial)
	{
		if (GameStatsCodex::GetIsDashed() && !GameStatsCodex::GetIsSkilled())
		{
			auto size = enemies.size();
			bool hasSkill = (pPlayer->GetWeaponSlots()->GetSlot(1).GetType() != WEAPON_TYPE_NONE);

			if (!Created && size <= 0 && !hasSkill)
			{
				enemies.push_back(ObjectCodex::Acquire<Enemy_Red_T>(character_Enemy_Red_T, gfx, rg));
				enemies.push_back(ObjectCodex::Acquire<Enemy_Green_T>(character_Enemy_Green_T, gfx, rg));
				enemies.push_back(ObjectCodex::Acquire<Enemy_Blue_T>(character_Enemy_Blue_T, gfx, rg));
				Created = true;
			}
			else if (size <= 0 && !hasSkill)
			{
				Created = false;
			}
			else if (size > 0 && hasSkill)
			{
				for (int i = 0; i < enemies.size(); i++) enemies[i]->SetIsDeath(true);
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
	}
	else
	{
		auto position = pPlayer->GetPosition();

#ifdef _DEBUG
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
		//if (NewEnemy)
		//{
		// // get new enemy type randomly
		//	std::uniform_int_distribution<int> enemyType(1, 6);
		//	type = enemyType(rng);
		// 
		// // get position around the player randomly
		std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<float> d(-30.0f, 30.0f);
		XMFLOAT3 pos = { position.x + d(rng),10.0f,position.z + d(rng) };
		if (pos.x > MapEdgeLeftAndRight) pos.x = MapEdgeLeftAndRight - 20.0f;
		if (pos.x < -MapEdgeLeftAndRight) pos.x = -MapEdgeLeftAndRight + 20.0f;
		if (pos.z > MapEdgeFrontAndBack) pos.z = MapEdgeFrontAndBack - 20.0f;
		if (pos.z < -MapEdgeFrontAndBack) pos.z = -MapEdgeFrontAndBack + 20.0f;

		//	NewEnemy = false;
		//}

		// 新しEnemyTestを生成する
		if (InputCodex::Get().KeyTriggered(KK_NUMPAD1) || type == 1) { enemies.push_back(ObjectCodex::Acquire<Enemy_Red_0>(character_Enemy_Red_0, gfx, rg, pos));	  Created = true; }
		if (InputCodex::Get().KeyTriggered(KK_NUMPAD4) || type == 4) { enemies.push_back(ObjectCodex::Acquire<Enemy_Red_1>(character_Enemy_Red_1, gfx, rg, pos));	  Created = true; }
		if (InputCodex::Get().KeyTriggered(KK_NUMPAD2) || type == 2) { enemies.push_back(ObjectCodex::Acquire<Enemy_Green_0>(character_Enemy_Green_0, gfx, rg, pos)); Created = true; }
		if (InputCodex::Get().KeyTriggered(KK_NUMPAD5) || type == 5) { enemies.push_back(ObjectCodex::Acquire<Enemy_Green_1>(character_Enemy_Green_1, gfx, rg, pos)); Created = true; }
		if (InputCodex::Get().KeyTriggered(KK_NUMPAD3) || type == 3) { enemies.push_back(ObjectCodex::Acquire<Enemy_Blue_0>(character_Enemy_Blue_0, gfx, rg, pos));	  Created = true; }
		if (InputCodex::Get().KeyTriggered(KK_NUMPAD6) || type == 6) { enemies.push_back(ObjectCodex::Acquire<Enemy_Blue_1>(character_Enemy_Blue_1, gfx, rg, pos));	  Created = true; }
		if (Created)
		{
			Created = false;
			enemies.back()->SpawnAt(pos);
			SoundCodex::Get().PlaySE3D(SndPath::SE_Enemy_Create, 0, 1.0f, pos.x, pos.y, pos.z);
		}
#else
		if (CountDown < Interval) 
		{
			CountDown += dt;
			if (pPlayer->GetIsFever())
				CountDown += dt * 3.0f;
		}
		if (CountDown >= Interval)
		{
			CountDown = 0.0f;
			NewEnemy = true;
		}
		int type = 0;
		if (NewEnemy)
		{
			std::mt19937 rng(std::random_device{}());

			// get new enemy type randomly
			std::uniform_int_distribution<int> enemyType(1, 6);
			type = enemyType(rng);

			// get position around the player randomly
			std::uniform_real_distribution<float> d(-30.0f, 30.0f);
			XMFLOAT3 pos = { position.x + d(rng),10.0f,position.z + d(rng) };
			if (pos.x > MapEdgeLeftAndRight) pos.x = MapEdgeLeftAndRight - 20.0f;
			if (pos.x < -MapEdgeLeftAndRight) pos.x = -MapEdgeLeftAndRight + 20.0f;
			if (pos.z > MapEdgeFrontAndBack) pos.z = MapEdgeFrontAndBack - 20.0f;
			if (pos.z < -MapEdgeFrontAndBack) pos.z = -MapEdgeFrontAndBack + 20.0f;

			NewEnemy = false;

			// 新しEnemyTestを生成する
			if (type == 1) { enemies.push_back(ObjectCodex::Acquire<Enemy_Red_0>(character_Enemy_Red_0, gfx, rg, pos));	  Created = true; }
			if (type == 4) { enemies.push_back(ObjectCodex::Acquire<Enemy_Red_1>(character_Enemy_Red_1, gfx, rg, pos));	  Created = true; }
			if (type == 2) { enemies.push_back(ObjectCodex::Acquire<Enemy_Green_0>(character_Enemy_Green_0, gfx, rg, pos)); Created = true; }
			if (type == 5) { enemies.push_back(ObjectCodex::Acquire<Enemy_Green_1>(character_Enemy_Green_1, gfx, rg, pos)); Created = true; }
			if (type == 3) { enemies.push_back(ObjectCodex::Acquire<Enemy_Blue_0>(character_Enemy_Blue_0, gfx, rg, pos));	  Created = true; }
			if (type == 6) { enemies.push_back(ObjectCodex::Acquire<Enemy_Blue_1>(character_Enemy_Blue_1, gfx, rg, pos));	  Created = true; }

			if (Created)
			{
				Created = false;
				enemies.back()->SpawnAt(pos);
				SoundCodex::Get().PlaySE3D(SndPath::SE_Enemy_Create, 0, 1.0f, pos.x, pos.y, pos.z);
			}
		}
#endif

		for (int i = 0; i < enemies.size(); i++)
		{
			// 対象かアクティブでいれば更新する、そうでない場合は容器から削除する
			if (enemies[i]->IsActive())
				enemies[i]->Update(dt);
			else
			{
				GameStatsCodex::AddTotalDefeat();
				switch (enemies[i]->GetEnemyType())
				{
				case ENEMY_TYPE_RED:	GameStatsCodex::AddRedDefeat();		break;
				case ENEMY_TYPE_GREEN:	GameStatsCodex::AddGreenDefeat();	break;
				case ENEMY_TYPE_BLUE:	GameStatsCodex::AddBlueDefeat();	break;
				}
				GameStatsCodex::AddScore((isInCD ? 1.0f : 0.1f) * enemies[i]->GetKillScore());

				enemies.erase(enemies.begin() + i);
				--i;
			}
		}
		//if (pEnemyBoss->IsActive()) pEnemyBoss->Update();
	}

	if (isTutorial && isTutorial != GameStatsCodex::GetIsTutorial())
	{
		isTutorial = GameStatsCodex::GetIsTutorial();
		Created = false;

		for (int i = 0; i < enemies.size(); i++) enemies[i]->SetIsDeath(true);
	}
}

void EnemyManager::Submit(void)
{
	for (int i = 0; i < enemies.size(); i++)
		if (enemies[i]->IsActive())
			enemies[i]->Submit();
	//if (pEnemyBoss->IsActive()) pEnemyBoss->Draw();
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