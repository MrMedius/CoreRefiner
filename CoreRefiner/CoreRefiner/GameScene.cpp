#include "GameScene.h"

#include "Channels.h"
#include "DeferredDisableQueue.h"
#include "GameRenderGraph.h"
#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "ObjectCodex.h"
#include "SoundCodex.h"

#include "AttackManager.h"
#include "EnemyManager.h"
#include "EnvironmentManager.h"
#include "ModuleWorkbench.h"
#include "Player.h"

#include "SceneDirector.h"
#include "UI_CombatHud.h"
#include "UI_Pause.h"
#include "UI_Prep.h"
#include "Window.h"

GameScene::GameScene(Window& wnd, SceneDirector& director, Rgph::GameRenderGraph& gameRG, std::function<void()> onSettings)
	:
	wnd_(wnd),
	director_(director),
	gameRG_(gameRG),
	onSettings_(std::move(onSettings)),
	light_(wnd.Gfx(), { 0.0f, 20.0f, 0.0f }),
	cameras_(wnd.Gfx()),
	pause_{ false },
	language_(GameStatsCodex::GetLanguage())
{
#ifdef _DEBUG
	cameras_.AddCamera(light_.ShareCamera());
#endif
	gameRG_.BindShadowCamera(*light_.ShareCamera());
	light_.LinkTechniques(gameRG_);
	cameras_.LinkTechniques(gameRG_);

	pPlayer_ = ObjectCodex::AcquirePersistent<Player>(character_Player, wnd.Gfx(), gameRG_, &cameras_, DirectX::XMFLOAT3{ 0.0f, 5.0f, 0.0f });
	pAttackManager_ = std::make_unique<AttackManager>(wnd.Gfx(), gameRG_);
	pEnvironmentManager_ = std::make_unique<EnvironmentManager>(wnd.Gfx(), gameRG_);
	pEnemyManager_ = std::make_unique<EnemyManager>(wnd.Gfx(), gameRG_);

	moduleWorkbench_ = std::make_unique<ModuleWorkbench>(wnd.Gfx(), gameRG_);
	uiPrep_ = std::make_unique<UI_Prep>(*moduleWorkbench_);
	uiPrep_->SetOnFight([this] { waveDirector_.RequestStartWave(); });

	uiPause_ = std::make_unique<UI_Pause>(wnd.Gfx(), gameRG_);
	uiPause_->SetOnContinue([this] { ClosePauseMenu_(); });
	uiPause_->SetOnSettings([this] {
		if (uiPause_ != nullptr)
		{
			uiPause_->Hide();
		}
		pause_ = true;
		if (onSettings_)
		{
			onSettings_();
		}
		});
	uiPause_->SetOnQuit([this] {
		ClosePauseMenu_();
		director_.RequestScene(SceneId::Title);
		});

	uiCombatHud_ = std::make_unique<UI_CombatHud>(wnd.Gfx(), gameRG_, &moduleWorkbench_->GetField());

	waveDirector_.onWaveStart = [this](const WaveSpec& spec)
	{
		EnterCombatPresentation_();
		pEnemyManager_->SetWave(spec);
	};
	waveDirector_.onWaveExpire = [this](int)
	{
		BeginVacuumSweep_();
	};
	waveDirector_.onWaveEnd = [this](int)
	{
		pEnvironmentManager_->CollectAllCoins();
		DeferredDisableQueue::Get().Flush();
		if (waveDirector_.GetPhase() == GamePhase::Prep)
		{
			EnterPrepPresentation_();
		}
	};
}

GameScene::~GameScene() = default;

void GameScene::OnEnter()
{
	GameStatsCodex::Reset();
	if (uiCombatHud_ != nullptr)
	{
		uiCombatHud_->Invalidate();
	}
	pEnvironmentManager_->EnterGame();
	if (pPlayer_ != nullptr)
	{
		pPlayer_->Activate();
	}
	cameras_.Reset();
	waveDirector_.Reset();
	waveDirector_.RequestStartWave();
	RefreshIfLanguageChanged_();
}

void GameScene::OnLeave()
{
	waveDirector_.Reset();
	if (moduleWorkbench_ != nullptr)
	{
		if (moduleWorkbench_->IsLayoutEditActive())
		{
			moduleWorkbench_->EndLayoutEdit();
		}
		moduleWorkbench_->Reset();
	}
	pAttackManager_->Reset();
	pEnemyManager_->Reset();
	pEnvironmentManager_->Reset();
	DeferredDisableQueue::Get().Flush();
	ClosePauseMenu_();
	wnd_.EnableCursor();
}

void GameScene::Update(float dt)
{
	RefreshIfLanguageChanged_();

	if (pause_ && uiPause_ != nullptr && !uiPause_->IsOpen())
	{
		uiPause_->Show();
		wnd_.EnableCursor();
	}

	const bool wasOpen = (uiPause_ != nullptr && uiPause_->IsOpen());

	if (InputCodex::Get().KeyTriggered(VK_ESCAPE))
	{
		TryTogglePauseMenu_();
	}

	if (uiPause_ != nullptr)
	{
		uiPause_->Update(dt);
	}

	pause_ = (uiPause_ != nullptr && uiPause_->IsOpen());
	if (pause_ || wasOpen)
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

void GameScene::Submit()
{
	light_.Bind(wnd_.Gfx(), cameras_->GetMatrix());
	gameRG_.BindMainCamera(cameras_.GetActiveCamera());

	if (IsCombatWorld_())
	{
#ifdef _DEBUG
		light_.Submit(Chan::main);
#endif
		cameras_.Submit(Chan::main);
		pAttackManager_->Submit();
		pEnemyManager_->Submit();
		pEnvironmentManager_->Submit();
		pPlayer_->Submit();
	}

	if (waveDirector_.GetPhase() == GamePhase::Prep)
	{
		if (uiPrep_ != nullptr)
		{
			uiPrep_->Submit();
		}
	}
	else if (uiCombatHud_ != nullptr)
	{
		uiCombatHud_->SubmitField();
		if (IsCombatWorld_())
		{
			uiCombatHud_->SubmitHud();
		}
	}

	if (uiPause_ != nullptr)
	{
		uiPause_->Submit();
	}

	gameRG_.Execute(wnd_.Gfx());

#ifdef _DEBUG
	if (!pause_ && IsCombatWorld_())
	{
		cameras_.SpawnWindow(wnd_.Gfx());
		light_.SpawnControlWindow();
		SoundCodex::Get().SpawnWindow();
		gameRG_.RenderWindows(wnd_.Gfx());
		GameStatsCodex::SpawnWindow();
	}
#endif
}

void GameScene::EnterCombatPresentation_()
{
	if (moduleWorkbench_ != nullptr && moduleWorkbench_->IsLayoutEditActive())
	{
		moduleWorkbench_->EndLayoutEdit();
	}
	if (pPlayer_ != nullptr)
	{
		pPlayer_->ApplyWaveStart();
	}
}

void GameScene::EnterPrepPresentation_()
{
	wnd_.EnableCursor();
	if (uiPrep_ == nullptr)
	{
		return;
	}
	uiPrep_->SetHostWindow(&wnd_);
	if (!uiPrep_->IsLayoutEditActive())
	{
		uiPrep_->BeginLayoutEdit();
	}
}

void GameScene::BeginVacuumSweep_()
{
	vacuumElapsed_ = 0.0f;
	pEnemyManager_->HaltSpawning();
	pEnemyManager_->ClearAll();
	pAttackManager_->Reset();
	pEnvironmentManager_->BeginVacuum();
}

void GameScene::TryFinishVacuum_(float dt)
{
	vacuumElapsed_ += dt;
	if (pEnvironmentManager_->HasActiveCoins() && vacuumElapsed_ < kVacuumTimeout_)
	{
		return;
	}
	waveDirector_.FinishWave();
}

void GameScene::TryNotifyPlayerDead_()
{
	if (pPlayer_ == nullptr || !pPlayer_->GetIsGameOver())
	{
		return;
	}
	waveDirector_.NotifyPlayerDead();
}

void GameScene::TryEnterResultIfTerminal_()
{
	if (!waveDirector_.IsTerminalPhase())
	{
		return;
	}
	if (waveDirector_.GetPhase() == GamePhase::Victory)
	{
		GameStatsCodex::SetGameClear();
	}
	director_.RequestScene(SceneId::Result);
}

bool GameScene::IsCombatWorld_() const noexcept
{
	const GamePhase phase = waveDirector_.GetPhase();
	return phase == GamePhase::Combat || phase == GamePhase::Vacuum;
}

void GameScene::ClosePauseMenu_() noexcept
{
	if (uiPause_ != nullptr)
	{
		uiPause_->Hide();
	}
	pause_ = false;
}

void GameScene::TryTogglePauseMenu_() noexcept
{
	if (uiPause_ == nullptr || waveDirector_.IsTerminalPhase())
	{
		return;
	}
	if (uiPause_->IsOpen())
	{
		uiPause_->Hide();
	}
	else
	{
		uiPause_->Show();
		wnd_.EnableCursor();
	}
	pause_ = uiPause_->IsOpen();
	InputCodex::Get().FlushKeyboard();
}

void GameScene::RefreshIfLanguageChanged_()
{
	const Language lang = GameStatsCodex::GetLanguage();
	if (lang == language_)
	{
		return;
	}
	language_ = lang;
	if (moduleWorkbench_ != nullptr)
	{
		moduleWorkbench_->RefreshFightLabel();
		moduleWorkbench_->GetShop().RefreshCopy();
	}
	if (uiCombatHud_ != nullptr)
	{
		uiCombatHud_->Invalidate();
	}
	if (uiPause_ != nullptr)
	{
		uiPause_->RefreshLabels();
	}
}

void GameScene::SyncCombatHud_()
{
	if (uiCombatHud_ == nullptr)
	{
		return;
	}
	uiCombatHud_->SetWave(waveDirector_.GetWaveIndex());
	uiCombatHud_->SetRemain(waveDirector_.GetRemainSec());
	if (pPlayer_ != nullptr)
	{
		uiCombatHud_->SetHpRatio(pPlayer_->GetHpDrawParameter());
	}
}

void GameScene::UpdateCombat_(float dt)
{
	auto playerPos = pPlayer_->GetPosition();
	cameras_.Update(dt, playerPos, &wnd_);
	if (!cameras_.GetScreenFroze())
	{
		light_.Update(dt, playerPos);

		pPlayer_->Update(dt);
		pAttackManager_->Update(dt);
		pEnvironmentManager_->Update(dt);
		pEnemyManager_->Update(dt);
	}
	gameRG_.Update(dt);
	SoundCodex::Get().SetListenerPosition(playerPos);

	if (moduleWorkbench_ != nullptr)
	{
		moduleWorkbench_->Update(dt, pAttackManager_.get());
	}
	SyncCombatHud_();
}

void GameScene::UpdateVacuum_(float dt)
{
	auto playerPos = pPlayer_->GetPosition();
	cameras_.Update(dt, playerPos, &wnd_);
	if (!cameras_.GetScreenFroze())
	{
		light_.Update(dt, playerPos);
		pPlayer_->Update(dt);
		pEnvironmentManager_->Update(dt);
	}
	gameRG_.Update(dt);
	SoundCodex::Get().SetListenerPosition(playerPos);
	SyncCombatHud_();
	TryFinishVacuum_(dt);
}

void GameScene::UpdatePrep_(float dt)
{
	uiPrep_->SetHostWindow(&wnd_);
	uiPrep_->SetNextWave(waveDirector_.GetNextWaveIndex());
	uiPrep_->Update(dt);
}