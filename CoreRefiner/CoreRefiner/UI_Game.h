#pragma once
#include "Graphics.h"
#include "GameStatsCodex.h"
#include "ObjectCodex.h"

#include "Player.h"
#include "Sprite2D.h"
#include "Sprite2DPolygon.h"
#include "UI_ResourceSlot.h"
#include "UI_WeaponSlot.h"
#include "UI_Score.h"
#include "UI_Combo.h"
#include "UI_Boss.h"
#include "UI_Tutorial.h"

#include "Channels.h"

class UI_Game
{
public:
	UI_Game(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		// Bg
		{
			pBg = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Hp_Low.png" });
			pBg->SetPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
			pBg->SetScale(SCREEN_WIDTH, SCREEN_HEIGHT);
			pBg->SetFrame(1, 1, 1, 1, 1, 0, false);
			pBg->LinkTechniques(rg);
		}
		// PlayerHpBar_Frame
		{
			pPlayerHpBar_Frame = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Player_HP_Bar.png" });
			pPlayerHpBar_Frame->SetPosition(200.0f, SCREEN_HEIGHT - 150.0f);
			pPlayerHpBar_Frame->SetScale(500.0f, 500.0f);
			pPlayerHpBar_Frame->SetFrame(12, 16, 12 * 16 - 1, 1, 1, 0, false);
			pPlayerHpBar_Frame->LinkTechniques(rg);
		}
		// PlayerHpBar_Bar
		{
			pPlayerHpBar_Bar = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Player_HP_Fill.png",
																						 "asset\\Images\\UI_Game\\Player_HP_Fever.png" });
			pPlayerHpBar_Bar->SetPosition(200.0f, SCREEN_HEIGHT - 68.0f);
			pPlayerHpBar_Bar->SetScale(300.0f, 40.0f);
			pPlayerHpBar_Bar->SetFrameAuto(8, 6, 1, 48, 0, 20.0f);
			pPlayerHpBar_Bar->LinkTechniques(rg);
		}
		// PlayerHpBar_Fire
		{
			pPlayerHpBar_Fire = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Player_HP_Bar.png" });
			pPlayerHpBar_Fire->SetPosition(200.0f, SCREEN_HEIGHT - 150.0f);
			pPlayerHpBar_Fire->SetScale(500.0f, 500.0f);
			pPlayerHpBar_Fire->SetFrameAuto(12, 16, 145, 46, 0, 20.0f);
			pPlayerHpBar_Fire->LinkTechniques(rg);
		}
		// PlayerHpBar_Side
		{
			pPlayerHpBar_Side = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Player_HP_Side.png" });
			pPlayerHpBar_Side->SetScale(15.0f, 50.0f);
			pPlayerHpBar_Side->SetFrameAuto(8, 6, 1, 48, 0, 30.0f);
			pPlayerHpBar_Side->LinkTechniques(rg);
		}
		// ResourceSlot
		for (size_t i = 0; i < slotNum; i++)
		{
			pResourceSlot[i] = std::make_unique<UI_ResourceSlot>(gfx, rg, static_cast<int>(i));
		}
		// WeaponSlot
		pWeaponSlot = std::make_unique<UI_WeaponSlot>(gfx, rg);
		// Combo
		pCombo = std::make_unique<UI_Combo>(gfx, rg);
		// Score
		pScore = std::make_unique<UI_Score>(gfx, rg);
		// Boss
		pBoss = std::make_unique<UI_Boss>(gfx, rg);
		// Tutorial
		pTutorial = std::make_unique<UI_Tutorial>(gfx, rg);
	}
	~UI_Game() = default;

	void Update(float dt)
	{
		auto* player = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
		bool fever = player->GetIsFever();

		// PlayerHpBar_Fire
		if (player->GetHpDrawParameter() < 0.5)
		{
			pPlayerHpBar_Fire->Update(dt);
		}

		if(!fever)
		{
			// PlayerHpBar_Frame
			pPlayerHpBar_Frame->SetFrame(12, 16, 12 * 16 - 1, 1, 1, 0, false);
			// PlayerHpBar_Bar
			pPlayerHpBar_Bar->SetTextureFrameIndex(0);
			pPlayerHpBar_Bar->Update(dt);
			pPlayerHpBar_Bar->SetRatioOffset(player->GetHpDrawParameter(), 1.0f);
			// PlayerHpBar_Side
			auto pos = pPlayerHpBar_Bar->GetPosition();
			auto size = pPlayerHpBar_Bar->GetScale();
			pPlayerHpBar_Side->SetPosition(pos.x + size.x / 2.0f, pos.y);
			pPlayerHpBar_Side->Update(dt);
			// ResourceSlot
			auto EnergyDrawParameters = player->GetResourceBars()->GetEnergyDrawParameters();
			auto EnergyFull = player->GetResourceBars()->GetEnergyFull();
			for (size_t i = 0; i < slotNum; i++)
			{
				pResourceSlot[i]->Update(dt, EnergyDrawParameters[i], EnergyFull[i], player->GetResourceBars()->GetEnergyChangedAt(i));
				if (EnergyFull[i]) player->GetResourceBars()->SetEnergyFullAt(i, false);
			}
			// WeaponSlot
			pWeaponSlot->Update(dt, player->GetWeaponSlots());
		}
		else
		{
			// PlayerHpBar_Frame
			pPlayerHpBar_Frame->SetFrame(12, 16, 12 * 16, 1, 1, 0, false);
			// PlayerHpBar_Bar
			pPlayerHpBar_Bar->SetTextureFrameIndex(1);
			pPlayerHpBar_Bar->Update(dt);
			pPlayerHpBar_Bar->SetRatioOffset(player->GetWeaponSlots()->GetFeverTimeDraw(), 1.0f);
			// PlayerHpBar_Side
			auto pos = pPlayerHpBar_Bar->GetPosition();
			auto size = pPlayerHpBar_Bar->GetScale();
			pPlayerHpBar_Side->SetPosition(pos.x + size.x / 2.0f, pos.y);
			pPlayerHpBar_Side->Update(dt);
		}
		// Combo
		{
			std::vector<unsigned int> combos = player->GetResourceBars()->GetAttackCombos();
			std::vector<float> countdowns = player->GetResourceBars()->GetAttackComboCountdownRates();
			pCombo->Update(dt, combos, countdowns);
		}
		if(!GameStatsCodex::GetIsTutorial())
		{
			// Score
			pScore->Update(dt);
			// Boss
			pBoss->Update(dt);
		}
		// Tutorial
		pTutorial->Update(dt);
	}

	void Submit(void)
	{
		auto* player = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
		bool fever = player->GetIsFever();

		if (player->GetHpDrawParameter() < 0.5)
		{
			// Bg
			pBg->Submit(Chan::ui);
			// PlayerHpBar_Fire
			pPlayerHpBar_Fire->Submit(Chan::ui);
		}
		// PlayerHpBar_Frame
		pPlayerHpBar_Frame->Submit(Chan::ui);
		// PlayerHpBar_HP
		pPlayerHpBar_Bar->Submit(Chan::ui);
		// PlayerHpBar_Side
		pPlayerHpBar_Side->Submit(Chan::ui);
		if (!fever)
		{
			if(!player->GetIsChange())
			{
				// ResourceSlot
				for (size_t i = 0; i < slotNum; i++)
				{
					pResourceSlot[i]->Submit();
				}
				// WeaponSlot
				pWeaponSlot->Submit();
			}
		}
		// Combo
		pCombo->Submit();

		if (!GameStatsCodex::GetIsTutorial())
		{
			// Score
			pScore->Submit();
			// Boss
			pBoss->Submit();
		}
		// Tutorial
		pTutorial->Submit();
	}

	void Reset(void)
	{
		// Score
		pScore->Reset();
		// Boss
		pBoss->Reset();
		// Tutorial
		pTutorial->Reset();
	}

	bool GetIsInCD() const
	{
		return pBoss->GetIsInCD();
	}

	bool GetIsVictory() const
	{
		return pBoss->GetIsVictory();
	}

private:
	// Bg
	std::unique_ptr<Sprite2D> pBg;
	// PlayerHpBar_Frame
	std::unique_ptr<Sprite2D> pPlayerHpBar_Frame;
	// PlayerHpBar_HP
	std::unique_ptr<Sprite2D> pPlayerHpBar_Bar;
	// PlayerHpBar_Fire
	std::unique_ptr<Sprite2D> pPlayerHpBar_Fire;
	// PlayerHpBar_Side
	std::unique_ptr<Sprite2D> pPlayerHpBar_Side;
	// ResourceSlot
	static constexpr int slotNum = 3;
	std::unique_ptr<UI_ResourceSlot> pResourceSlot[slotNum];
	// WeaponSlot
	std::unique_ptr<UI_WeaponSlot> pWeaponSlot;
	// Combo
	std::unique_ptr<UI_Combo> pCombo;
	// Score
	std::unique_ptr<UI_Score> pScore;
	// Boss
	std::unique_ptr<UI_Boss> pBoss;
	// Tutorial
	std::unique_ptr<UI_Tutorial> pTutorial;
};