#pragma once
#include "Graphics.h"
#include "Sprite2D.h"
#include "Channels.h"

class UI_Combo
{
public:
	UI_Combo(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		comboNum.resize(slotNum);
		comboActive.resize(slotNum);
		pCombo.resize(slotNum);
		for (size_t i = 0; i < slotNum; i++)
		{
			// Combo_Bg
			{
				float posY = 205.0f;
				float offset = 80.0f;
				pBg.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Combo_Bg.png" }));
				pBg[i]->SetPosition(basePosX, posY + offset * i);
				pBg[i]->SetScale(160.0f, 40.0f);
				pBg[i]->SetFrame(3, 1, i + 1, 1, 1, 0);
				pBg[i]->LinkTechniques(rg);
			}
			// Combo_Countdown
			{
				float posY = 230.0f;
				float offset = 80.0f;
				pCD.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Combo_Countdown.png" }));
				pCD[i]->SetPosition(160.0f, posY + offset * i);
				pCD[i]->SetScale(250.0f, 50.0f);
				pCD[i]->SetFrame(3, 1, i + 1, 1, 1, 0);
				pCD[i]->LinkTechniques(rg);
			}
			// Combo_Count
			pCombo[i].reserve(maxDigit);
			comboNum[i].resize(maxDigit, 0);
			for (size_t j = 0;j < maxDigit;j++)
			{
				pCombo[i].push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Combo_Number.png" }));
				pCombo[i][j]->SetScale(baseSize, baseSize);
				pCombo[i][j]->LinkTechniques(rg);
			}
		}
	}
	~UI_Combo() = default;

	void Update(float dt, std::vector<unsigned int> combos, std::vector<float> countdowns)
	{
		for (size_t i = 0; i < slotNum; i++)
		{
			// check if has combo
			comboActive[i] = false;
			std::fill(comboNum[i].begin(), comboNum[i].end(), 0);
			int combo = combos[i];
			float countdown = countdowns[i];

			// has combo
			if (combo == 0) continue;
			// has no combo
			else
			{
				// set combo active
				comboActive[i] = true;

				// offset rate
				float rate = 0.15f;

				// Combo_Bg
				{
					auto pos = pBg[i]->GetPosition();
					// offset
					if( 1.0f - countdown < rate)
					{
						pos.x = basePosX * (countdown + rate);
						pBg[i]->SetPosition(pos.x, pos.y);
					}
				}
				// Combo_Countdown
				{
					pCD[i]->SetRatioOffset(countdown, 1.0f);
				}
				// Combo_Count
				{
					// threshold
					if (combo > 999) combo = 999;
					// calculate combo situation
					int maxCombo = 1;
					for (int t = 0; t < maxDigit; ++t) maxCombo *= 10;
					if (combo >= maxCombo) combo = maxCombo - 1;
					size_t k = 0;
					while (combo > 0 && k < maxDigit)
					{
						comboNum[i][k] = combo % 10;
						combo /= 10;
						++k;
					}
					// set parameters
					float posX = 80.0f;
					float offset = 25.0f;
					float posY = 205.0f + static_cast<float>(i) * 80.0f;
					for (size_t j = 0; j < maxDigit; j++)
					{
						pCombo[i][j]->SetPosition(posX - offset * j, posY);
						pCombo[i][j]->SetFrame(5, 6, i * 10 + comboNum[i][j] + 1, 1, 1, 0, false);
						if (1.0f - countdown < rate)
						{
							float size = baseSize * (countdown + rate);
							pCombo[i][j]->SetScale(size, size);
						}
					}
				}
			}
		}
	}
	void Submit(void)
	{
		for (size_t i = 0; i < slotNum; i++)
		{
			if (!comboActive[i]) continue;
			// Combo_Bg
			pBg[i]->Submit(Chan::ui);
			// Combo_Countdown
			pCD[i]->Submit(Chan::ui);
			// Combo_Count
			int highest = maxDigit - 1;
			while (highest > 0 && comboNum[i][highest] == 0) --highest;
			for (int j = 0; j <= highest; ++j) pCombo[i][j]->Submit(Chan::ui);
		}
	}
private:
	static constexpr int slotNum = 3;
	static constexpr int maxDigit = 3;
	std::vector<bool> comboActive;
	// Combo_Bg
	static constexpr float basePosX = 180.0f;
	std::vector<std::unique_ptr<Sprite2D>> pBg;
	// Combo_Countdown
	std::vector<std::unique_ptr<Sprite2D>> pCD;
	// Combo_Count
	static constexpr float baseSize = 40.0f;
	std::vector<std::vector<int>> comboNum;
	std::vector<std::vector<std::unique_ptr<Sprite2D>>> pCombo;
};