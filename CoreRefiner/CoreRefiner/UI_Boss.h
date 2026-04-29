#pragma once
#include "Graphics.h"
#include "Sprite2D.h"
#include "Channels.h"
#include "SoundCodex.h"

class UI_Boss
{
public:
	UI_Boss(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		state = Countdown;
		// Boss_HP_Frame
		{
			pFrame = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Boss_HP_Frame.png" });
			pFrame->SetPosition(SCREEN_WIDTH / 2.0f, 102.0f);
			pFrame->SetScale(620.0f, 30.0f);
			pFrame->SetFrame(1, 1, 1, 1, 1, 0, false);
			pFrame->LinkTechniques(rg);
		}
		// Boss_CD
		{
			cdRemain = 0.0f;
			pCD = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ 
				"asset\\Images\\UI_Game\\Boss_Countdown.png",
				"asset\\Images\\UI_Game\\Boss_Appear.png",
				"asset\\Images\\UI_Game\\Boss_Fight.png"});
			pCD->SetPosition(SCREEN_WIDTH / 2.0f, 100.0f);
			pCD->SetScale(600.0f, 30.0f);
			pCD->SetFrameAuto(3, 16, 1, 48, 0, 20.0f);
			pCD->LinkTechniques(rg);
		}
		// Boss_Side
		{
			pSide = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Boss_HP_Side.png" });
			pSide->SetScale(15.0f, 30.0f);
			pSide->SetFrameAuto(8, 6, 1, 48, 0, 30.0f);
			pSide->LinkTechniques(rg);
		}
	}

	~UI_Boss() = default;

	void Update(float dt)
	{
		// Boss_CD
		pCD->Update(dt);

		switch (state)
		{
		case UI_Boss::Countdown:
		{
			{
				// Boss_CD
				cdRemain += dt;
				if (cdRemain >= cdTotal && isInCD) isInCD = false;
				pCD->SetRatioOffset(cdRemain / cdTotal, 1.0f);
			}

			if (!isInCD)
			{
				state = Appear;
				pCD->SetFrameAuto(3, 4, 1, 12, 1, 20.0f);
			}
		}
			break;
		case UI_Boss::Appear:
		{
			if (pCD->ClipFinished())
			{
				state = Fight;
				cdRemain = cdTotal;
				pCD->SetFrameAuto(4, 37, 1, 148, 2, 20.0f);
			}
		}
			break;
		case UI_Boss::Fight:
		{
			// Boss_CD
			cdRemain -= dt;
			pCD->SetRatioOffset(cdRemain / cdTotal, 1.0f);
		}
			break;
		}

		// Boss_Side
		{
			auto pos = pCD->GetPosition();
			auto size = pCD->GetScale();
			pSide->SetPosition(pos.x + size.x / 2.0f, pos.y);
			pSide->Update(dt);
		}
	}

	void Submit(void)
	{
		// Boss_HP_Frame
		if(state == Fight) pFrame->Submit(Chan::ui);
		// Boss_CD
		pCD->Submit(Chan::ui);
		// Boss_Side
		if (state == Countdown) pSide->Submit(Chan::ui);
	}

	void Reset(void)
	{
		state = Countdown;
		// Boss_CD
		cdRemain = 0.0f;
		isInCD = true;
		pCD->SetRatioOffset(0.0f, 1.0f);
		pCD->SetFrameAuto(3, 16, 1, 48, 0, 20.0f);
	}

	bool GetIsInCD() const
	{
		return isInCD;
	}

	bool GetIsVictory() const
	{
		return state == Fight && cdRemain <= 0.0f;
	}
private:
	enum State
	{
		Countdown,
		Appear,
		Fight,
	} state{ Countdown };
	// Boss_HP_Frame
	std::unique_ptr<Sprite2D> pFrame;
	// Boss_CD
	static constexpr float cdTotal = 60.0f;
	float cdRemain{ 0.0f };
	bool isInCD{ true };
	std::unique_ptr<Sprite2D> pCD;
	// Boss_Side
	std::unique_ptr<Sprite2D> pSide;
};