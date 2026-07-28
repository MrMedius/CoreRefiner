#include "Game.h"
#include "imgui/imgui.h"
#include "Util.h"
#include "Channels.h"

#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "SoundCodex.h"
#include "ObjectCodex.h"
#include "TimeCodex.h"
#include "DeferredDisableQueue.h"

namespace dx = DirectX;

/*
 * Scene leave / enter checklist (Phase 6.1)
 *
 * Leave SCENE_GAME:
 *   - AttackManager::Reset  — Deactivate active balls, clear manager list (pool keeps instances)
 *   - EnemyManager::Reset   — Deactivate active enemies, clear manager list
 *   - EnvironmentManager::Reset — Deactivate Field (persistent tag; no ClearTag)
 *   - Flush DeferredDisableQueue
 *   - Player: DO NOT Deactivate / ClearTag (AcquirePersistent)
 *   - Camera / Light: keep; reset on Enter
 *
 * Enter SCENE_GAME:
 *   - EnvironmentManager::EnterGame — Activate Field
 *   - Player::Activate — runs OnEnable reset (spawn pose / HP / FSM)
 *   - CameraContainer::Reset
 *
 * Leave/Enter TITLE / RESULT:
 *   - UI only for now; no Codex ClearAll
 *
 * Never use ObjectCodex::ClearAll / ClearTag for normal scene switches.
 */

Game::Game(const std::string& commandLine)
	:
	commandLine(commandLine),
	wnd(1280, 720, "CR"),
	scriptCommander(TokenizeQuoted(commandLine)),
	light(wnd.Gfx(), { 0.0f,20.0f,0.0f })
{
#ifdef _DEBUG
	cameras.AddCamera(light.ShareCamera());
#endif
	gameRG.BindShadowCamera(*light.ShareCamera());
	light.LinkTechniques(gameRG);
	cameras.LinkTechniques(gameRG);

	// Persistent player (survives scene leave)
	pPlayer = ObjectCodex::AcquirePersistent<Player>(character_Player, wnd.Gfx(), gameRG, &cameras, XMFLOAT3{ 0.0f,20.0f,0.0f });
	pAttackManager = std::make_unique<AttackManager>(wnd.Gfx(), gameRG);
	pEnvironmentManager = std::make_unique<EnvironmentManager>(wnd.Gfx(), gameRG);
	pEnemyManager = std::make_unique<EnemyManager>(wnd.Gfx(), gameRG);


	// UI
	uiTitle = std::make_unique<UI_Title>(wnd.Gfx(), UIRG);
	uiTitle->SetOnNewGame([this] { SetScene(SCENE_GAME); });

	uiSample = std::make_unique<UI_Sample>(wnd.Gfx(), UIRG);

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
			const float dt = timer_update.Mark() * speed_factor;
			TimeCodex::Get().Update(dt);
			Update(dt);
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

void Game::SetScene(SCENE scene)
{
	if (scene == Scene)
	{
		return;
	}
	LeaveScene(Scene);
	Scene = scene;
	EnterScene(Scene);
}

void Game::LeaveScene(SCENE scene)
{
	switch (scene)
	{
	case SCENE_GAME:
		pAttackManager->Reset();
		pEnemyManager->Reset();
		pEnvironmentManager->Reset();
		DeferredDisableQueue::Get().Flush();
		// Player remains AcquirePersistent — not Deactivated here
		break;
	case SCENE_TITLE:
	case SCENE_RESULT:
	default:
		break;
	}
}

void Game::EnterScene(SCENE scene)
{
	switch (scene)
	{
	case SCENE_GAME:
		pEnvironmentManager->EnterGame();
		if (pPlayer != nullptr)
		{
			pPlayer->Activate();
		}
		cameras.Reset();
		break;
	case SCENE_TITLE:
	case SCENE_RESULT:
	default:
		break;
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

	// Simple scene-cycle test: Shift → TITLE → GAME → RESULT → TITLE ...
	if (InputCodex::Get().KeyTriggered(VK_SHIFT))
	{
		const SCENE next = static_cast<SCENE>((static_cast<int>(Scene) + 1) % static_cast<int>(SCENE_COUNT));
		SetScene(next);
	}

	switch (Scene)
	{
	case SCENE_TITLE:
		//uiTitle->Update(dt);
		uiSample->Update(dt);
		break;
	case SCENE_GAME:
		if (!Pause)
		{
			// Game Loop
			auto playerPos = pPlayer->GetPosition();
			cameras.Update(dt, playerPos, &wnd);
			if (!cameras.GetScreenFroze())
			{
				light.Update(dt, playerPos);

				pPlayer->Update(dt);
				pAttackManager->Update(dt);
				pEnvironmentManager->Update(dt);
				pEnemyManager->Update(dt);
			}
			gameRG.Update(dt);
			SoundCodex::Get().SetListenerPosition(playerPos);
		}
		break;
	case SCENE_RESULT:
		break;
	}

	// Frame-end: deferred Deactivate (RequestDisable / SetDestroy equivalent)
	DeferredDisableQueue::Get().Flush();

	//UpdateCanvasDemo(dt);
}

void Game::Draw()
{
	switch (Scene)
	{
	case SCENE_TITLE:
	{
		//uiTitle->Submit();
		uiSample->Submit();

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
		pAttackManager->Submit();
		pEnemyManager->Submit();
		pEnvironmentManager->Submit();
		pPlayer->Submit();

		//pTestCanvasWorld->Submit(Chan::main | Chan::shadow);

		gameRG.Execute(wnd.Gfx());

#ifdef _DEBUG
		// imgui windows
		cameras.SpawnWindow(wnd.Gfx());
		light.SpawnControlWindow();
		SoundCodex::Get().SpawnWindow();

		gameRG.RenderWindows(wnd.Gfx());
#endif
		break;
	}
	case SCENE_RESULT:
	{
		UIRG.Execute(wnd.Gfx());
		break;
	}
	}
}
