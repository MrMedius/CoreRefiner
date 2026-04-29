#include "EffectManager.h"
#include "ObjectCodex.h"
#include <random>
#include <algorithm>

#include "Enemy.h"
#include "Effect_Player_Remote_1.h"
#include "Effect_Player_Remote_2.h"
#include "Effect_Player_Remote_3.h"
#include "Effect_Player_Hit.h"
#include "Effect_Player_EnergyAbsorb.h"

EffectManager::EffectManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

	for (int i = 0;i < 30;i++)
	{
		ObjectCodex::Acquire<Effect_Player_Remote_1>(effect_Player_Remote_Attack_1, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, false);
		ObjectCodex::Acquire<Effect_Player_Remote_2>(effect_Player_Remote_Attack_2, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, false);
		ObjectCodex::Acquire<Effect_Player_Remote_3>(effect_Player_Remote_Attack_3, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, false);
		ObjectCodex::Acquire<Effect_Player_Hit>(effect_Player_Hit, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, false, 0);
		ObjectCodex::Acquire<Effect_Player_EnergyAbsorb>(effect_Player_EnergyAbsorb, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f }, 1);
	}
	for (int i = 0;i < 30;i++)
	{
		ObjectCodex::FindFirstActiveObjectByTag<Effect_Player_Remote_1>(effect_Player_Remote_Attack_1)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Effect_Player_Remote_2>(effect_Player_Remote_Attack_2)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Effect_Player_Remote_3>(effect_Player_Remote_Attack_3)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Effect_Player_Hit>(effect_Player_Hit)->Deactivate();
		ObjectCodex::FindFirstActiveObjectByTag<Effect_Player_EnergyAbsorb>(effect_Player_EnergyAbsorb)->Deactivate();
	}
}

void EffectManager::Update(float dt)
{
	// create player's remote attack effect
	if (pPlayer->GetIsAttack())
	{
		if (pPlayer->GetWeaponSlots()->GetCurrentSlot().GetType() == WEAPON_TYPE_2)
		{
			auto type = pPlayer->GetDoAttackType();

			if (type != Attack_Type_None && playerAttackType != type)
			{
				playerAttackType = type;
				auto pos = pPlayer->GetAttackPosition();
				auto flip = pPlayer->GetIsFlip();

				switch (playerAttackType)
				{
				case Player_Attack_1: effects.push_back(ObjectCodex::Acquire<Effect_Player_Remote_1>(effect_Player_Remote_Attack_1, gfx, rg, pos, flip)); playerRemote++; break;
				case Player_Attack_2: effects.push_back(ObjectCodex::Acquire<Effect_Player_Remote_2>(effect_Player_Remote_Attack_2, gfx, rg, pos, flip)); playerRemote++; break;
				case Player_Attack_3: effects.push_back(ObjectCodex::Acquire<Effect_Player_Remote_3>(effect_Player_Remote_Attack_3, gfx, rg, pos, flip)); playerRemote++; break;
				}
				if (effects.size() > 0) effects.back()->SpawnAt(pos, flip);
			}
		}
	}
	else
	{
		playerAttackType = Attack_Type_None;
	}

	// create player's hit effect
	if (pPlayer->GetAttackCollisionOnOff() || playerRemote > 0)
	{
		std::vector<Enemy*> enemies;
		for (auto tag : {
			character_Enemy_Red_T,
			character_Enemy_Red_0,
			character_Enemy_Red_1,
			character_Enemy_Green_T,
			character_Enemy_Green_0,
			character_Enemy_Green_1,
			character_Enemy_Blue_T,
			character_Enemy_Blue_0,
			character_Enemy_Blue_1
			}) {
			// プレイヤーを攻撃できる全ての敵を探す
			auto found = ObjectCodex::FindActiveObjectsByTag<Enemy>(tag);
			// 全ての敵を整理する
			enemies.reserve(enemies.size() + found.size());
			enemies.insert(enemies.end(), found.begin(), found.end());
		}

		for (auto e : enemies)
		{
			if (e->GetIsHurt() && e->GetWasHurt())
			{
				auto pos = e->GetPosition();
				pos = { pos.x,pos.y, pos.z - 0.1f };
				auto flip = e->GetIsFlip();
				auto type = static_cast<int>(e->GetEnemyType());
				int count = pPlayer->GetResourceBars()->GetAttackCombos()[type - 1];
				count = std::clamp(count, 5, 25) / 5;

				auto* ea = ObjectCodex::Acquire<Effect_Player_EnergyAbsorb>(effect_Player_EnergyAbsorb, gfx, rg, pos, type);
				effects.push_back(ea);
				ea->SpawnWithType(pos, type, count);

				type = 0;
				if (pPlayer->GetWeaponSlots()->GetCurrentSlot().GetType() == WEAPON_TYPE_1) type = 1;
				else if (pPlayer->GetWeaponSlots()->GetCurrentSlot().GetType() == WEAPON_TYPE_3) type = 3;
				if (type == 1 || type == 3)
				{
					auto* eh = ObjectCodex::Acquire<Effect_Player_Hit>(effect_Player_Hit, gfx, rg, pos, flip, type);
					effects.push_back(eh);
					eh->SpawnWithType(pos, flip, type);
				}
			}
		}
	}

	// update all effects
	for (int i = 0; i < effects.size(); i++)
	{
		// 対象かアクティブでいれば更新する、そうでない場合は容器から削除する
		if (effects[i]->IsActive())
			effects[i]->Update(dt);
		else
		{
			const auto tag = effects[i]->GetTag();
			if (tag == effect_Player_Remote_Attack_1 ||
				tag == effect_Player_Remote_Attack_2 ||
				tag == effect_Player_Remote_Attack_3)
			{
				if (playerRemote > 0) --playerRemote;
			}

			effects.erase(effects.begin() + i);
			--i;
		}
	}
}

void EffectManager::Submit(void)
{
	for (int i = 0; i < effects.size(); i++)
		if (effects[i]->IsActive())		
			effects[i]->Submit();
}

void EffectManager::Reset(void)
{
	for (int i = 0; i < effects.size(); i++)
		if (effects[i]->IsActive())		
			effects[i]->Deactivate();

	effects.clear();

	playerRemote = 0;
}