#include "Game.h"
#include <memory>
#include <algorithm>
#include "Math.h"
#include "Surface.h"
#include "imgui/imgui.h"
#include "VertexBuffer.h"
#include "Util.h"
#include "Camera.h"
#include "Channels.h"
#include "ObjectBase.h"

#include "InputCodex.h"
#include "SoundCodex.h"
#include "ObjectCodex.h"
#include "GameStatsCodex.h"


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


	InitCanvasDemo();
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

	UpdateCanvasDemo(dt);
}

void Game::Draw()
{
	switch (Scene)
	{
	case SCENE_TITLE:
	{
		pTestCanvasUi->Submit(Chan::ui);
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

		pTestCanvasWorld->Submit(Chan::main | Chan::shadow);

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


void Game::InitCanvasDemo()
{
	{
		const unsigned cw = 64u, ch = 64u;
		pTestCanvasUi = std::make_unique<Canvas2D>(wnd.Gfx(), cw, ch);

		for (unsigned y = 0; y < ch; ++y)
		{
			for (unsigned x = 0; x < cw; ++x)
			{
				pTestCanvasUi->PutPixel(x, y, ((x ^ y) & 8u) ? Colors::Cyan : Colors::None);
			}
		}

		pTestCanvasUi->SetPosition({ 100.0f, 100.0f, 0.0f });
		pTestCanvasUi->SetScale({ 196.0f, 196.0f, 1.0f });
		pTestCanvasUi->LinkTechniques(UIRG);
	}

	{
		const unsigned cw = 64u, ch = 64u;
		pTestCanvasWorld = std::make_unique<Canvas3D>(wnd.Gfx(), cw, ch);

		for (unsigned y = 0; y < ch; ++y)
		{
			for (unsigned x = 0; x < cw; ++x)
			{
				pTestCanvasWorld->PutPixel(x, y, ((x ^ y) & 8u) ? Colors::Yellow : Colors::None);
			}
		}

		pTestCanvasWorld->SetPosition({ 0.0f, 14.0f, 10.0f });
		pTestCanvasWorld->SetScale({ 4.0f, 4.0f, 1.0f });
		pTestCanvasWorld->LinkTechniques(gameRG);
	}
}


void Game::UpdateCanvasDemo(float dt)
{
	InitCanvasDemo();

	canvasAnimT += dt;
	if (!pTestCanvasUi || !pTestCanvasWorld)
		return;

	const unsigned w = pTestCanvasUi->GetCanvasWidth();
	const unsigned h = pTestCanvasUi->GetCanvasHeight();
	if (w == 0u || h == 0u)
		return;

	const unsigned x = static_cast<unsigned>((sinf(canvasAnimT * 2.0f) * 0.5f + 0.5f) * float(w - 1u));
	const unsigned y = static_cast<unsigned>((cosf(canvasAnimT * 2.0f) * 0.5f + 0.5f) * float(h - 1u));

	for (int i = -10; i < 10; i++)
	{
		pTestCanvasUi->PutPixel(x + i, y + i, Colors::Red);
		pTestCanvasWorld->PutPixel(x + i, y + i, Colors::Green);
		pTestCanvasUi->PutPixel(x - i, y + i, Colors::Red);
		pTestCanvasWorld->PutPixel(x - i, y + i, Colors::Green);
	}
	
}