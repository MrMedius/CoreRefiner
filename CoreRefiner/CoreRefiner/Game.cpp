#include "Game.h"
#include <memory>
#include <algorithm>
#include "Math.h"
#include "Surface.h"
#include "imgui/imgui.h"
#include "VertexBuffer.h"
#include "Util.h"
#include "PerfLog.h"
#include "Camera.h"
#include "Channels.h"
#include "ModelWindow.h"
#include "ObjectBase.h"

#include "InputCodex.h"
#include "SoundCodex.h"
#include "ObjectCodex.h"
#include "GameStatsCodex.h"

namespace dx = DirectX;

Game::Game(const std::string& commandLine)
	:
	commandLine(commandLine),
	wnd(1280, 720, "PRIMARY"),
	scriptCommander(TokenizeQuoted(commandLine)),
	light(wnd.Gfx(), { 0.0f,70.0f,0.0f })
{
#ifdef _DEBUG
	cameras.AddCamera(light.ShareCamera());
#endif
	gameRG.BindShadowCamera(*light.ShareCamera());
	light.LinkTechniques(gameRG);
	cameras.LinkTechniques(gameRG);

	// Objects
	pPlayer = ObjectCodex::Acquire<Player>(character_Player, wnd.Gfx(), gameRG, &cameras, XMFLOAT3{ 0.0f,45.0f,0.0f });
	pEnvironmentManager = std::make_unique<EnvironmentManager>(wnd.Gfx(), gameRG);
	pEnemyManager = std::make_unique<EnemyManager>(wnd.Gfx(), gameRG);
	pEffectManager = std::make_unique<EffectManager>(wnd.Gfx(), gameRG);

	// UI
	pUI_Title = std::make_unique<UI_Title>(wnd.Gfx(), UIRG);
	pUI_Game = std::make_unique<UI_Game>(wnd.Gfx(), gameRG);
	pUI_Result = std::make_unique<UI_Result>(wnd.Gfx(), UIRG);
	pUI_Loading = std::make_unique<UI_Loading>(wnd.Gfx(), UIRG);

	// Sound Base Setting
	SoundCodex::Get().PlayBGM(SndPath::BGM_Title, -1);
	SoundCodex::Get().SetBgmVolume(0.1f);
	SoundCodex::Get().SetSeVolume(1.0f);
	SoundCodex::Get().SetListenerTransform(0.0f, 0.0f, 0.0f, 0, 0, 1, 0, 1, 0);
}

Game::~Game()
{}

int Game::RunGame()
{
	while( true )
	{
		// process all messages pending, but to not block for new messages
		if( const auto ecode = Window::ProcessMessages() )
		{
			// if return optional has value, means we're quitting so return exit code
			return *ecode;
		}

		if (timer_update.Peek() >= 1.0f / 60.0f) // Execute at max 60 FPS
		{
#ifdef _DEBUG
			FrameCounter++;
			// FPS Calculation and display in Debug Title
			if (timer_FPS.Peek() >= 1.0f)
			{
				wnd.SetDebugTitle(FrameCounter);
				FrameCounter = 0;
				timer_FPS.Mark();
			}
#endif
			/********************************/
			/*            Update            */
			wnd.TickCursorAutoHide();
			wnd.Gfx().BeginFrame();
			Update(timer_update.Mark() * speed_factor);
			InputCodex::Get().Update();
			SoundCodex::Get().Update();
			/********************************/

			/********************************/
			/*             Draw             */
			Draw();
			wnd.Gfx().EndFrame(); // present
			gameRG.Reset();
			UIRG.Reset();
			/********************************/
		}
	}
}

void Game::Update(float dt)
{
#ifdef _DEBUG
	if( InputCodex::Get().KeyTriggered( VK_F11 ) )
	{
		wnd.ToggleFullscreen();
	}
#endif

	switch (Scene)
	{
	case SCENE_TITLE:
		pUI_Title->Update(dt);

		if (pUI_Title->GetIsStart())
		{
			pUI_Title->Reset();
			SetScene(SCENE_LOADING);
			pUI_Loading->StartLoading(SCENE_GAME);

			SoundCodex::Get().PlayBGM(SndPath::BGM_Game, -1);
		}
		if (pUI_Title->GetIsFullscreen())
		{
			wnd.ToggleFullscreen();
			pUI_Title->ResetFullscreen();
		}
		if (pUI_Title->GetIsEnd())
		{
			PostQuitMessage(0);
		}
		break;
	case SCENE_GAME:
		if (!Pause)
		{
			// Check If Game Finished
			if (pPlayer->GetIsGameOver() || pUI_Game->GetIsVictory())
			{
				if (pUI_Game->GetIsVictory())
				{
					SoundCodex::Get().PlaySE(SndPath::SE_Game_Victory);
					GameStatsCodex::SetGameClear();
				}
				if (pPlayer->GetIsGameOver())
				{
					SoundCodex::Get().PlaySE(SndPath::SE_Game_Over);
				}
				pUI_Result->Reset();
				SetScene(SCENE_LOADING);
				pUI_Loading->StartLoading(SCENE_RESULT);

				SoundCodex::Get().PlayBGM(SndPath::BGM_Result, -1);
			}

			// Game Loop
			auto playerPos = pPlayer->GetPosition();
			cameras.Update(dt, playerPos, &wnd);
			if (!cameras.GetScreenFroze())
			{
				light.Update(dt, playerPos);

				pPlayer->Update(dt);

				if (!pPlayer->GetIsChange())
				{
					GameStatsCodex::UpdateLifeTime(dt);
					pUI_Game->Update(dt);

					pEnvironmentManager->Update(dt);
					pEnemyManager->Update(dt, pUI_Game->GetIsInCD());
					pEffectManager->Update(dt);
				}
			}
			gameRG.Update(dt);
			SoundCodex::Get().SetListenerPosition(playerPos);
		}
		break;
	case SCENE_RESULT:
		pUI_Result->Update(dt);

		if (pUI_Result->GetToTitle())
		{
			SetScene(SCENE_LOADING);
			pUI_Loading->StartLoading(SCENE_TITLE);

			pPlayer->OnEnable();
			cameras.Reset();
			light.Reset();
			pEnemyManager->Reset();
			pEnvironmentManager->Reset();
			pEffectManager->Reset();
			pUI_Game->Reset();
			GameStatsCodex::Reset();
			SoundCodex::Get().PlayBGM(SndPath::BGM_Title, -1);
		}
		break;
	case SCENE_LOADING:
		pUI_Loading->Update(dt);

		if (pUI_Loading->FinishLoading())
		{
			SetScene(static_cast<SCENE>(pUI_Loading->SceneLoading()));
		}

		break;
	}	
}

void Game::Draw()
{
	switch (Scene)
	{
	case SCENE_TITLE:
	{
		// UI
		pUI_Title->Submit();

		UIRG.Execute(wnd.Gfx());
		break;
	}
	case SCENE_GAME:
	{
		light.Bind(wnd.Gfx(), cameras->GetMatrix());
		gameRG.BindMainCamera(cameras.GetActiveCamera());
#ifdef _DEBUG
		light.Submit(Chan::main);
#endif
		cameras.Submit(Chan::main);
		// Objects
		pEnvironmentManager->Submit();
		pEnemyManager->Submit();
		pEffectManager->Submit();
		pPlayer->Submit();
		// UI
		pUI_Game->Submit();

		gameRG.Execute(wnd.Gfx());

#ifdef _DEBUG
		// imgui windows
		cameras.SpawnWindow(wnd.Gfx());
		light.SpawnControlWindow();
		pEnvironmentManager->SpawnWindow();
		SoundCodex::Get().SpawnWindow();
		GameStatsCodex::SpawnWindow();

		gameRG.RenderWindows(wnd.Gfx());
#endif
		break;
	}
	case SCENE_RESULT:
	{
		pUI_Result->Submit();

		UIRG.Execute(wnd.Gfx());
		break;
	}
	case SCENE_LOADING:
	{
		pUI_Loading->Submit();

		UIRG.Execute(wnd.Gfx());
		break;
	}
	}
}