#pragma once
#include <memory>
#include <unordered_map>
#include "Graphics.h"
#include "Button.h"
#include "Sprite2D.h"
#include "Channels.h"

#include "InputCodex.h"

class UI_Title
{
private:
	enum Title_Button_Type_Tag
	{
		None,

		Start,
		Fullscreen,
		End,
	};
public:
	UI_Title(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		// Background
		pBackground = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Title\\UI_Title.png" });
		pBackground->SetPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
		pBackground->SetScale(SCREEN_WIDTH, SCREEN_HEIGHT);
		pBackground->SetFrame(1, 1, 1, 1, 1, 0, false);
		pBackground->LinkTechniques(rg);
		// Button_Bg
		pButtonBg = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Title\\UI_Title_Button_Bg.png" });
		pButtonBg->SetPosition(300.0f, SCREEN_HEIGHT / 2);
		pButtonBg->SetScale(SCREEN_HEIGHT, SCREEN_HEIGHT);
		pButtonBg->SetFrameAuto(12, 12, 1, 144, 0, 60.0f);
		pButtonBg->LinkTechniques(rg);
		// Logo
		pLogo = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Title\\UI_Title_Logo.png" });
		pLogo->SetPosition(310.0f, 150.0f);
		pLogo->SetRotation(-5.0f);
		pLogo->SetScale(600.0f, 600.0f);
		pLogo->SetFrame(1, 1, 1, 1, 1, 0, false);
		pLogo->LinkTechniques(rg);
		// Buttons
		float posX = 275.0f;
		XMFLOAT2 size = { 250.0f,250.0f };
		XMFLOAT2 check = { 200.0f,100.0f };
		pButtons[Start] =		std::make_unique<Button>(gfx, rg, XMFLOAT2(posX, 400.0f), size, check, 3, 2, 3, 3, 6, "asset\\Images\\UI_Title\\UI_Title_Buttons.png");
		pButtons[Fullscreen] =	std::make_unique<Button>(gfx, rg, XMFLOAT2(posX, 500.0f), size, check, 3, 2, 2, 2, 5, "asset\\Images\\UI_Title\\UI_Title_Buttons.png");
		pButtons[End] =			std::make_unique<Button>(gfx, rg, XMFLOAT2(posX, 600.0f), size, check, 3, 2, 1, 1, 4, "asset\\Images\\UI_Title\\UI_Title_Buttons.png");
		// Button_Frame
		pButtonFrame = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Title\\UI_Title_Button_Frame.png" });
		pButtonFrame->SetScale(400.0, 400.0f);
		pButtonFrame->SetFrameAuto(12, 12, 1, 144, 0, 30.0f);
		pButtonFrame->LinkTechniques(rg);
	}
	~UI_Title() = default;

	void Update(float dt)
	{
		auto& input = InputCodex::Get();

		// Buttons
		{
			if (input.MouseMovedOrButtons()) // mouse
			{
				XMFLOAT2 mousePos = { static_cast<float>(input.MouseX()),static_cast<float>(input.MouseY()) };

				bool haveChoose = false;
				for (auto& b : pButtons)
					if (b.second->MouseEnterCheck(mousePos))
					{
						chosenButtonTag = b.first;
						b.second->SetOnChoose(true);
						b.second->SetOnClick(input.MouseLeftPressed());

						haveChoose = true;
					}
					else
						b.second->SetOnChoose(false);

				if (!haveChoose) chosenButtonTag = None;
			}
			else // keyboard
			{
				if (input.KeyTriggered(KK_S) || input.GP_Triggered(0, Gamepad::GP_DPAD_DOWN) || input.GP_LeftY(0) < 0)
					if (chosenButtonTag != End && chosenButtonTag < End)
						chosenButtonTag = static_cast<Title_Button_Type_Tag>(chosenButtonTag + 1);
				if (input.KeyTriggered(KK_W) || input.GP_Triggered(0, Gamepad::GP_DPAD_UP) || input.GP_LeftY(0) > 0)
					if (chosenButtonTag != Start && chosenButtonTag > Start)
						chosenButtonTag = static_cast<Title_Button_Type_Tag>(chosenButtonTag - 1);
				for (auto& b : pButtons)
				{
					if (b.first == chosenButtonTag)
					{
						b.second->SetOnChoose(true);
						b.second->SetOnClick(input.KeyPressed(KK_ENTER) || input.GP_Pressed(0, Gamepad::GP_A));
					}
					else
						b.second->SetOnChoose(false);
				}
			}
			if (chosenButtonTag != None)
			{
				if (input.MouseLeftReleased() || input.KeyReleased(KK_ENTER) || input.GP_Released(0, Gamepad::GP_A))
				{
					pButtons[chosenButtonTag]->SetOnExecute(true);
					OnExecute = true;
				}
			}
			if (OnExecute)
			{
				switch (chosenButtonTag)
				{
				case UI_Title::Start:
					IsStart = true;
					break;
				case UI_Title::Fullscreen:
					IsFullscreen = true;
					break;
				case UI_Title::End:
					IsEnd = true;
					break;
				}
				OnExecute = false;
			}
		}
		// Button_Bg
		pButtonBg->Update(dt);
		// Button_Frame
		if (chosenButtonTag != None)
		{
			auto pos = pButtons.find(chosenButtonTag)->second->GetPosition();
			pButtonFrame->SetPosition(pos.x + 25.0f, pos.y);
		}
		pButtonFrame->Update(dt);
	}

	void Submit(void)
	{		
		// Background
		pBackground->Submit(Chan::ui);
		// Button_Bg
		pButtonBg->Submit(Chan::ui);
		// Logo
		pLogo->Submit(Chan::ui);
		// Buttons
		for (auto& b : pButtons) b.second->Submit();
		// Button_Frame
		if (chosenButtonTag != None) pButtonFrame->Submit(Chan::ui);
	}

	void Reset(void)
	{
		chosenButtonTag = None;
		OnExecute = false;
		IsStart = false;
		IsFullscreen = false;
		IsEnd = false;
		for (auto& b : pButtons) b.second->Reset();
	}

	bool GetIsStart() const { return IsStart; }
	bool GetIsFullscreen() const { return IsFullscreen; }
	void ResetFullscreen() { IsFullscreen = false; }
	bool GetIsEnd() const { return IsEnd; }
private:
	// Background
	std::unique_ptr<Sprite2D> pBackground;
	// Button_Bg
	std::unique_ptr<Sprite2D> pButtonBg;
	// Logo
	std::unique_ptr<Sprite2D> pLogo;
	// Buttons
	std::unordered_map<Title_Button_Type_Tag, std::unique_ptr<Button>> pButtons;
	Title_Button_Type_Tag chosenButtonTag{ None };
	bool OnExecute{ false };
	bool IsStart{ false };
	bool IsFullscreen{ false };
	bool IsEnd{ false };
	// Button_Frame
	std::unique_ptr<Sprite2D> pButtonFrame;
};
