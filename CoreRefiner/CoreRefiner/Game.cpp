#include "Game.h"
#include "imgui/imgui.h"
#include "Util.h"
#include "Channels.h"

#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "SoundCodex.h"
#include "ObjectCodex.h"
#include "TextCodex.h"

#include "ButtonViewModel.h"
#include "FocusManager.h"

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

	// Objects
	pPlayer = ObjectCodex::Acquire<Player>(character_Player, wnd.Gfx(), gameRG, &cameras, XMFLOAT3{ 0.0f,15.0f,0.0f });
	pAttackManager = std::make_unique<AttackManager>(wnd.Gfx(), gameRG);
	pEnvironmentManager = std::make_unique<EnvironmentManager>(wnd.Gfx(), gameRG);
	pEnemyManager = std::make_unique<EnemyManager>(wnd.Gfx(), gameRG);

	// Sound Base Setting
	SoundCodex::Get().PlayBGM(SndPath::BGM_Title, -1);
	SoundCodex::Get().SetBgmVolume(0.1f);
	SoundCodex::Get().SetSeVolume(1.0f);
	SoundCodex::Get().SetListenerTransform(0.0f, 0.0f, 0.0f, 0, 0, 1, 0, 1, 0);



	UITestInit();
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
		uiRoot.TickAfterInput();

		if (InputCodex::Get().KeyTriggered(VK_SPACE))
		{
			SetScene(SCENE_GAME);
		}
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

	//UpdateCanvasDemo(dt);
}

void Game::Draw()
{
	switch (Scene)
	{
	case SCENE_TITLE:
	{
		uiRoot.Submit(Chan::ui);

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




void Game::UITestInit()
{
	constexpr Ui::FocusHandle kFocusBtnA = 501u;
	constexpr Ui::FocusHandle kFocusBtnB = 502u;

	constexpr unsigned kCanvasLogicalW = 380u;
	constexpr unsigned kCanvasLogicalH = 100u;

	btnA_ = std::make_unique<Ui::UiButton>(
		kFocusBtnA,
		Ui::UiRect{ 200.0f, 300.0f, 580.0f, 400.0f });
	btnB_ = std::make_unique<Ui::UiButton>(
		kFocusBtnB,
		Ui::UiRect{ 700.0f, 300.0f, 1080.0f, 400.0f });

	btnA_->SetLabel("Btn A | clicks 0");
	btnB_->SetLabel("Btn B | clicks 0");

	btnA_->SetOnClick([this] {
		++clicksA_;
		btnA_->SetLabel("Btn A | clicks " + std::to_string(clicksA_));
		});
	btnB_->SetOnClick([this] {
		++clicksB_;
		btnB_->SetLabel("Btn B | clicks " + std::to_string(clicksB_));
		});

	viewA_ = std::make_unique<Ui::ButtonCanvasView>(wnd.Gfx(), kCanvasLogicalW, kCanvasLogicalH);
	viewB_ = std::make_unique<Ui::ButtonCanvasView>(wnd.Gfx(), kCanvasLogicalW, kCanvasLogicalH);

	// Quad center / scale align with logical hit boxes (logical canvas coords).
	viewA_->GetCanvas().SetPosition({ 390.0f, 350.0f, 0.0f });
	viewA_->GetCanvas().SetScale({ 380.0f, 100.0f, 1.0f });
	viewB_->GetCanvas().SetPosition({ 890.0f, 350.0f, 0.0f });
	viewB_->GetCanvas().SetScale({ 380.0f, 100.0f, 1.0f });

	auto va = Ui::MakeButtonViewModel(*btnA_);
	auto vb = Ui::MakeButtonViewModel(*btnB_);
	viewA_->SyncFrom(va);
	viewB_->SyncFrom(vb);

	uiRoot.Clear();
	uiRoot.AddButtonSlot(btnA_.get(), viewA_.get());
	uiRoot.AddButtonSlot(btnB_.get(), viewB_.get());
	uiRoot.RebuildTabOrderFromSlots();
	uiRoot.InitLinkTechniques(UIRG);
}