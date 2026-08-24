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
	pPlayer = ObjectCodex::AcquirePersistent<Player>(character_Player, wnd.Gfx(), gameRG, &cameras, XMFLOAT3{ 0.0f,5.0f,0.0f });
	pAttackManager = std::make_unique<AttackManager>(wnd.Gfx(), gameRG);
	pEnvironmentManager = std::make_unique<EnvironmentManager>(wnd.Gfx(), gameRG);
	pEnemyManager = std::make_unique<EnemyManager>(wnd.Gfx(), gameRG);


	// UI
	moduleWorkbench = std::make_unique<ModuleWorkbench>(wnd.Gfx(), gameRG);
	uiTitle = std::make_unique<UI_Title>(wnd.Gfx(), UIRG);
	uiTitle->SetOnNewGame([this] { SetScene(SCENE_GAME); });
	uiPrep = std::make_unique<UI_Prep>(*moduleWorkbench);
	uiPrep->SetOnFight([this] { waveDirector_.RequestStartWave(); });
	uiCombatHud = std::make_unique<UI_CombatHud>(wnd.Gfx(), gameRG, &moduleWorkbench->GetField());
	uiSample = std::make_unique<UI_Sample>(wnd.Gfx(), UIRG);

	waveDirector_.onWaveStart = [this](const WaveSpec& spec)
	{
		EnterCombatPresentation_();
		pEnemyManager->SetWave(spec);
	};
	waveDirector_.onWaveExpire = [this](int)
	{
		BeginVacuumSweep_();
	};
	waveDirector_.onWaveEnd = [this](int)
	{
		pEnvironmentManager->CollectAllCoins();
		DeferredDisableQueue::Get().Flush();
		if (waveDirector_.GetPhase() == GamePhase::Prep)
		{
			EnterPrepPresentation_();
		}
	};

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
		waveDirector_.Reset();
		// Discard in-progress assemble (standby/pending) before live attack list.
		if (moduleWorkbench != nullptr)
		{
			if (moduleWorkbench->IsLayoutEditActive())
			{
				moduleWorkbench->EndLayoutEdit();
			}
			moduleWorkbench->Reset();
		}
		pAttackManager->Reset();
		pEnemyManager->Reset();
		pEnvironmentManager->Reset();
		DeferredDisableQueue::Get().Flush();
		Pause = false;
		wnd.EnableCursor();
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
		waveDirector_.Reset();
		waveDirector_.RequestStartWave();
		break;
	case SCENE_TITLE:
	case SCENE_RESULT:
	default:
		break;
	}
}

void Game::EnterCombatPresentation_()
{
	if (moduleWorkbench != nullptr && moduleWorkbench->IsLayoutEditActive())
	{
		moduleWorkbench->EndLayoutEdit();
	}
}

void Game::EnterPrepPresentation_()
{
	wnd.EnableCursor();
	if (uiPrep == nullptr)
	{
		return;
	}
	uiPrep->SetHostWindow(&wnd);
	if (!uiPrep->IsLayoutEditActive())
	{
		uiPrep->BeginLayoutEdit();
	}
}

void Game::BeginVacuumSweep_()
{
	vacuumElapsed_ = 0.0f;
	pEnemyManager->HaltSpawning();
	pEnemyManager->ClearAll();
	pAttackManager->Reset();
	pEnvironmentManager->BeginVacuum();
}

void Game::TryFinishVacuum_(float dt)
{
	vacuumElapsed_ += dt;
	if (pEnvironmentManager->HasActiveCoins() && vacuumElapsed_ < kVacuumTimeout_)
	{
		return;
	}
	waveDirector_.FinishWave();
}

bool Game::IsCombatWorld_() const noexcept
{
	const GamePhase phase = waveDirector_.GetPhase();
	return phase == GamePhase::Combat || phase == GamePhase::Vacuum;
}

void Game::SyncCombatHud_()
{
	if (uiCombatHud == nullptr)
	{
		return;
	}
	uiCombatHud->SetWave(waveDirector_.GetWaveIndex());
	uiCombatHud->SetRemain(waveDirector_.GetRemainSec());
	if (pPlayer != nullptr)
	{
		uiCombatHud->SetHpRatio(pPlayer->GetHpDrawParameter());
	}
}

void Game::UpdateGameScene_(float dt)
{
	if (InputCodex::Get().KeyTriggered(KK_P))
	{
		waveDirector_.DebugSkipPhase();
	}

	if (Pause)
	{
		return;
	}

	waveDirector_.Update(dt);
	switch (waveDirector_.GetPhase())
	{
	case GamePhase::Combat:
		UpdateCombat_(dt);
		break;
	case GamePhase::Vacuum:
		UpdateVacuum_(dt);
		break;
	case GamePhase::Prep:
		UpdatePrep_(dt);
		break;
	case GamePhase::Defeat:
	case GamePhase::Victory:
	default:
		break;
	}
}

void Game::UpdateCombat_(float dt)
{
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

	if (moduleWorkbench != nullptr)
	{
		moduleWorkbench->Update(dt, pAttackManager.get());
	}
	SyncCombatHud_();
}

void Game::UpdateVacuum_(float dt)
{
	auto playerPos = pPlayer->GetPosition();
	cameras.Update(dt, playerPos, &wnd);
	if (!cameras.GetScreenFroze())
	{
		light.Update(dt, playerPos);
		pPlayer->Update(dt);
		pEnvironmentManager->Update(dt);
	}
	gameRG.Update(dt);
	SoundCodex::Get().SetListenerPosition(playerPos);
	SyncCombatHud_();
	TryFinishVacuum_(dt);
}

void Game::UpdatePrep_(float dt)
{
	uiPrep->SetHostWindow(&wnd);
	uiPrep->SetNextWave(waveDirector_.GetNextWaveIndex());
	uiPrep->Update(dt);
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
		uiTitle->Update(dt);
		break;
	case SCENE_GAME:
		UpdateGameScene_(dt);
		break;
	case SCENE_RESULT:
		uiSample->Update(dt);
		break;
	}

	// Frame-end: deferred Deactivate (RequestDisable / SetDestroy equivalent)
	DeferredDisableQueue::Get().Flush();
}

void Game::Draw()
{
	switch (Scene)
	{
	case SCENE_TITLE:
	{
		uiTitle->Submit();

		UIRG.Execute(wnd.Gfx());
		break;
	}
	case SCENE_GAME:
	{
		light.Bind(wnd.Gfx(), cameras->GetMatrix());
		gameRG.BindMainCamera(cameras.GetActiveCamera());

		if (!Pause && IsCombatWorld_())
		{
#ifdef _DEBUG
			light.Submit(Chan::main);
#endif
			cameras.Submit(Chan::main);
			pAttackManager->Submit();
			pEnemyManager->Submit();
			pEnvironmentManager->Submit();
			pPlayer->Submit();
		}

		if (waveDirector_.GetPhase() == GamePhase::Prep)
		{
			if (uiPrep != nullptr)
			{
				uiPrep->Submit();
			}
		}
		else if (uiCombatHud != nullptr)
		{
			uiCombatHud->SubmitField();
			if (!Pause && IsCombatWorld_())
			{
				uiCombatHud->SubmitHud();
			}
		}

		gameRG.Execute(wnd.Gfx());

#ifdef _DEBUG
		if (!Pause && IsCombatWorld_())
		{
			cameras.SpawnWindow(wnd.Gfx());
			light.SpawnControlWindow();
			SoundCodex::Get().SpawnWindow();
			gameRG.RenderWindows(wnd.Gfx());
			GameStatsCodex::SpawnWindow();
		}
#endif
		break;
	}
	case SCENE_RESULT:
	{
		uiSample->Submit();

		UIRG.Execute(wnd.Gfx());
		break;
	}
	}
}
