#include "Game.h"
#include "imgui/imgui.h"
#include "Util.h"
#include "Channels.h"

#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "SoundCodex.h"
#include "SettingsStore.h"
#include "DisplaySettings.h"
#include "UiCopy.h"
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

	(void)LoadSettings();
	ApplyWindowSizeIndex(wnd, GameStatsCodex::GetWindowSizeIndex());
	ApplyFullscreen(wnd, GameStatsCodex::GetFullscreen());

	// Persistent player (survives scene leave)
	pPlayer = ObjectCodex::AcquirePersistent<Player>(character_Player, wnd.Gfx(), gameRG, &cameras, XMFLOAT3{ 0.0f,5.0f,0.0f });
	pAttackManager = std::make_unique<AttackManager>(wnd.Gfx(), gameRG);
	pEnvironmentManager = std::make_unique<EnvironmentManager>(wnd.Gfx(), gameRG);
	pEnemyManager = std::make_unique<EnemyManager>(wnd.Gfx(), gameRG);

	// UI
	(void)LoadUiCopy(); // LoadJsonTexts
	moduleWorkbench = std::make_unique<ModuleWorkbench>(wnd.Gfx(), gameRG);
	uiTitle = std::make_unique<UI_Title>(wnd.Gfx(), uiRG);
	uiTitle->SetOnNewGame([this] { SetScene(SCENE_GAME); });
	uiTitle->SetOnSettings([this] { OpenSettings_(SettingsReturn::Title); });
	uiPrep = std::make_unique<UI_Prep>(*moduleWorkbench);
	uiPrep->SetOnFight([this] { waveDirector_.RequestStartWave(); });
	uiPause = std::make_unique<UI_Pause>(wnd.Gfx(), gameRG);
	uiPause->SetOnContinue([this] { ClosePauseMenu_(); });
	uiPause->SetOnSettings([this] {
		if (uiPause != nullptr)
		{
			uiPause->Hide();
		}
		Pause = true;
		OpenSettings_(SettingsReturn::Pause);
		});
	uiPause->SetOnQuit([this] { ClosePauseMenu_(); SetScene(SCENE_TITLE); });
	uiSetting = std::make_unique<UI_Setting>(wnd.Gfx(), uiRG);
	uiSetting->SetOnBack([this] { CloseSettings_(); });
	uiSetting->SetOnFullscreenChanged([this](bool on) { ApplyFullscreen(wnd, on); });
	uiSetting->SetOnWindowSizeIndex([this](int index) { ApplyWindowSizeIndex(wnd, index); });
	uiCombatHud = std::make_unique<UI_CombatHud>(wnd.Gfx(), gameRG, &moduleWorkbench->GetField());
	uiSample = std::make_unique<UI_Sample>(wnd.Gfx(), uiRG);

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
	SoundCodex::Get().SetMasterVolume(
		GameStatsCodex::GetMuted() ? 0.0f : GameStatsCodex::GetMasterVolume());
	SoundCodex::Get().SetBgmVolume(GameStatsCodex::GetBgmVolume());
	SoundCodex::Get().SetSeVolume(GameStatsCodex::GetSeVolume());
	SoundCodex::Get().SetListenerTransform(0.0f, 0.0f, 0.0f, 0, 0, 1, 0, 1, 0);
}

Game::~Game()
{
	(void)SaveSettings();
}

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
			uiRG.Reset();
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
	DismissSettings_();
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
		ClosePauseMenu_();
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
		GameStatsCodex::Reset();
		if (uiCombatHud != nullptr)
		{
			uiCombatHud->Invalidate();
		}
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
	if (pPlayer != nullptr)
	{
		pPlayer->ApplyWaveStart();
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

void Game::TryNotifyPlayerDead_()
{
	if (pPlayer == nullptr || !pPlayer->GetIsGameOver())
	{
		return;
	}
	waveDirector_.NotifyPlayerDead();
}

void Game::TryEnterResultIfTerminal_()
{
	if (!waveDirector_.IsTerminalPhase())
	{
		return;
	}
	if (waveDirector_.GetPhase() == GamePhase::Victory)
	{
		GameStatsCodex::SetGameClear();
	}
	SetScene(SCENE_RESULT);
}

bool Game::IsCombatWorld_() const noexcept
{
	const GamePhase phase = waveDirector_.GetPhase();
	return phase == GamePhase::Combat || phase == GamePhase::Vacuum;
}

void Game::ClosePauseMenu_() noexcept
{
	if (uiPause != nullptr)
	{
		uiPause->Hide();
	}
	Pause = false;
}

void Game::TryTogglePauseMenu_() noexcept
{
	if (uiPause == nullptr || waveDirector_.IsTerminalPhase())
	{
		return;
	}
	if (uiSetting != nullptr && uiSetting->IsOpen())
	{
		return;
	}
	if (uiPause->IsOpen())
	{
		uiPause->Hide();
	}
	else
	{
		uiPause->Show();
		wnd.EnableCursor();
	}
	Pause = uiPause->IsOpen();
	InputCodex::Get().FlushKeyboard();
}

void Game::OpenSettings_(SettingsReturn from)
{
	if (uiSetting == nullptr)
	{
		return;
	}
	settingsReturn_ = from;
	wnd.EnableCursor();
	uiSetting->Show();
	InputCodex::Get().FlushKeyboard();
}

void Game::CloseSettings_()
{
	if (uiSetting == nullptr || !uiSetting->IsOpen())
	{
		return;
	}
	uiSetting->Hide();

	if (moduleWorkbench != nullptr)
	{
		moduleWorkbench->RefreshFightLabel();
		moduleWorkbench->GetShop().RefreshCopy();
	}
	if (uiCombatHud != nullptr)
	{
		uiCombatHud->Invalidate();
	}
	if (uiTitle != nullptr)
	{
		uiTitle->RefreshLabels();
	}
	if (uiPause != nullptr)
	{
		uiPause->RefreshLabels();
	}
	uiSetting->RefreshLabels();

	if (settingsReturn_ == SettingsReturn::Pause && uiPause != nullptr)
	{
		uiPause->Show();
		Pause = true;
	}
	(void)SaveSettings();
	InputCodex::Get().FlushKeyboard();
}

void Game::DismissSettings_() noexcept
{
	if (uiSetting != nullptr)
	{
		uiSetting->Hide();
	}
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
	const bool wasOpen = (uiPause != nullptr && uiPause->IsOpen());

	if (InputCodex::Get().KeyTriggered(VK_ESCAPE))
	{
		TryTogglePauseMenu_();
	}

	if (uiPause != nullptr)
	{
		uiPause->Update(dt);
	}

	Pause = (uiPause != nullptr && uiPause->IsOpen());
	if (Pause || wasOpen)
	{
		return;
	}

#ifdef _DEBUG
	if (InputCodex::Get().KeyTriggered(KK_P))
	{
		waveDirector_.DebugSkipPhase();
	}
#endif

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
	TryNotifyPlayerDead_();
	TryEnterResultIfTerminal_();
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
	const bool settingsOpen = (uiSetting != nullptr && uiSetting->IsOpen());

	if (settingsOpen)
	{
		if (InputCodex::Get().KeyTriggered(VK_ESCAPE))
		{
			CloseSettings_();
		}
		if (uiSetting != nullptr && uiSetting->IsOpen())
		{
			uiSetting->Update(dt);
		}
		DeferredDisableQueue::Get().Flush();
		return;
	}

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
		if (uiSetting != nullptr && uiSetting->IsOpen())
		{
			uiSetting->Submit();
		}
		else
		{
			uiTitle->Submit();
		}
		uiRG.Execute(wnd.Gfx());
		break;
	}
	case SCENE_GAME:
	{
		if (uiSetting != nullptr && uiSetting->IsOpen())
		{
			uiSetting->Submit();
			uiRG.Execute(wnd.Gfx());
			break;
		}

		light.Bind(wnd.Gfx(), cameras->GetMatrix());
		gameRG.BindMainCamera(cameras.GetActiveCamera());

		if (IsCombatWorld_())
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
			if (IsCombatWorld_())
			{
				uiCombatHud->SubmitHud();
			}
		}

		if (uiPause != nullptr)
		{
			uiPause->Submit();
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

		uiRG.Execute(wnd.Gfx());
		break;
	}
	}
}
