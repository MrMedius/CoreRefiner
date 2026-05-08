#include "Game.h"
#include "imgui/imgui.h"
#include "Util.h"
#include "Channels.h"

#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "SoundCodex.h"
#include "ObjectCodex.h"
#include "TextCodex.h"

#include "TextBlock.h"
#include "Colors.h"

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
		const unsigned cw = 1000u, ch = 500u;
		pTestCanvasUi = std::make_unique<Canvas2D>(wnd.Gfx(), cw, ch);

		//for (unsigned y = 0; y < ch; ++y)
		//{
		//	for (unsigned x = 0; x < cw; ++x)
		//	{
		//		pTestCanvasUi->PutPixel(x, y, ((x ^ y) & 8u) ? Colors::Cyan : Colors::None);
		//	}
		//}

		pTestCanvasUi->SetPosition({ 500.0f, 300.0f, 0.0f });
		pTestCanvasUi->SetScale({ 1000.0f, 500.0f, 1.0f });
		pTestCanvasUi->LinkTechniques(UIRG);
	}

	{
		const unsigned cw = 320u, ch = 320u;
		pTestCanvasWorld = std::make_unique<Canvas3D>(wnd.Gfx(), cw, ch);

		//for (unsigned y = 0; y < ch; ++y)
		//{
		//	for (unsigned x = 0; x < cw; ++x)
		//	{
		//		pTestCanvasWorld->PutPixel(x, y, ((x ^ y) & 8u) ? Color(100u, 100u, 0u, 50u) : Colors::None);
		//	}
		//}

		pTestCanvasWorld->SetPosition({ 0.0f, 30.0f, 10.0f });
		pTestCanvasWorld->SetScale({ 40.0f, 40.0f, 1.0f });
		pTestCanvasWorld->LinkTechniques(gameRG);
	}
}




// 每帧调用也行；这里做静态缓存避免反复重建
static void Test_TextBlock_ModeA_FixedCanvas(Canvas& canvas)
{
	static TextBlock tb;
	static bool inited = false;
	if (!inited)
	{
		TextBlock::Style st{};
		st.fontFamily = L"Segoe UI";
		st.fontSize = 20.0f;
		st.weight = DWRITE_FONT_WEIGHT_NORMAL;
		st.wrapping = DWRITE_WORD_WRAPPING_WRAP;
		st.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		tb.SetStyle(st);
		tb.SetPadding(6);

		tb.SetTextUtf8(
			"哥们儿，这瓜多少钱一斤呐？"
			"两块钱一斤。"
			"这瓜皮子是金子做的，还是瓜粒子是金子做的？"
			"你瞧瞧这现在哪有瓜呀？这都是大棚的瓜，你嫌贵我还嫌贵呢。"
			"给我挑一个。"
			"行，这个怎么样？"
			"这瓜保熟吗？"
			"我开水果摊儿的，能卖给你生瓜蛋子啊？\n"
			"我问你这瓜保熟吗？\n"
			"你是故意找岔儿，是不是？你要不要吧！\n"
			"你这瓜要熟我肯定要啊。那它要是不熟怎么办呀？\n"
			"哎，要是不熟，我自己吃了它，满意了吧？\n"
			"15斤，30块。\n"
			"你这哪够15斤哪？你这称有问题呀。\n"
			"你故意找茬儿是不是？\n"
			"你要不要吧？你要不要？\n"
			"吸铁石，另外你说的，这瓜要是生的，你自己吞进去啊。\n"
			"你劈我瓜是吧！\n"
			"欻！刺！\n"
			"撒日朗！撒日朗！\n"
		);
		inited = true;
	}
	// 关键：固定画布尺寸，不会改变纹理宽高比 -> 不会因 SetScale 拉伸
	tb.RenderToCanvasFixed(canvas, Colors::White);
}
static void Test_TextBlock_ModeB_AutoSized(Canvas& canvas)
{
	static TextBlock tb;
	static bool inited = false;
	if (!inited)
	{
		TextBlock::Style st{};
		st.fontFamily = L"Segoe UI";
		st.fontSize = 28.0f;
		st.wrapping = DWRITE_WORD_WRAPPING_WRAP;
		tb.SetStyle(st);
		tb.SetPadding(6);
		// 关键：模式B固定maxWidth像素宽，高度自适应
		tb.SetMaxWidth(1000.0f);
		tb.SetTextUtf8(
			"哥们儿，这瓜多少钱一斤呐？"
			"两块钱一斤。"
			"这瓜皮子是金子做的，还是瓜粒子是金子做的？"
			"你瞧瞧这现在哪有瓜呀？这都是大棚的瓜，你嫌贵我还嫌贵呢。"
			"给我挑一个。"
			"行，这个怎么样？"
			"这瓜保熟吗？"
			"我开水果摊儿的，能卖给你生瓜蛋子啊？\n"
			"我问你这瓜保熟吗？\n"
			"你是故意找岔儿，是不是？你要不要吧！\n"
			"你这瓜要熟我肯定要啊。那它要是不熟怎么办呀？\n"
			"哎，要是不熟，我自己吃了它，满意了吧？\n"
			"15斤，30块。\n"
			"你这哪够15斤哪？你这称有问题呀。\n"
			"你故意找茬儿是不是？\n"
			"你要不要吧？你要不要？\n"
			"吸铁石，另外你说的，这瓜要是生的，你自己吞进去啊。\n"
			"你劈我瓜是吧！\n"
			"欻！刺！\n"
			"撒日朗！撒日朗！\n"
		);
		inited = true;
	}
	tb.RenderToCanvasAuto(canvas, Colors::White);
	// 关键：RenderToCanvasAuto 会改变 canvas 像素宽高比，
	// 所以你显示时必须等比缩放（否则一定变形）。
	// 下面只是“思路示例”：显示宽固定为 300，则显示高按像素比计算：
	const float w = float(canvas.GetCanvasWidth());
	const float h = float(canvas.GetCanvasHeight());
	const float displayW = 50.0f;
	const float displayH = (w > 0.0f) ? (displayW * (h / w)) : 50.0f;
	canvas.SetScale({ displayW, displayH, 1.0f });
}

static void Test_RichText_Spans(Canvas& canvas)
{
	static TextBlock tb;
	static bool inited = false;
	if (!inited)
	{
		TextBlock::Style st{};
		st.fontFamily = L"Segoe UI";
		st.fontSize = 20.0f;
		st.wrapping = DWRITE_WORD_WRAPPING_WRAP;
		tb.SetStyle(st);
		tb.SetPadding(6);
		// 注意：start/length 是按 UTF-16 code unit 的索引（wstring长度），而不是 UTF-8 字节。
		// 最简单的测试方法：先用纯 ASCII 文本来验证 span，然后再扩展到中文/日文。
		tb.SetTextUtf8(
			"MAN! What can I say?\n"
			"MANBA OUT!\n"
			"It's bing a long day without you, my friend\n"
		);
		std::vector<TextSpan> spans;
		// "RED"
		spans.push_back(TextSpan{
			.start = 0, .length = 20,
			.color = Colors::Yellow
			});
		// "Bold"
		spans.push_back(TextSpan{
			.start = 0, .length = 20,
			.weight = DWRITE_FONT_WEIGHT_BOLD
			});
		// "BLUE"
		spans.push_back(TextSpan{
			.start = 21, .length = 10,
			.color = Colors::Chart[10][5]
			});
		spans.push_back(TextSpan{
			.start = 32, .length = 43,
			.color = Colors::Red
			});
		tb.SetSpans(std::move(spans));
		inited = true;
	}
	tb.RenderToCanvasFixed(canvas, Colors::White);
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


	TextCodex::Get().DrawLine_SystemFont(*pTestCanvasUi, "あSystem123中文化:Segoe UI ABC", "SimSun", 30.0f, DWRITE_FONT_WEIGHT_NORMAL, 0, 30, Colors::White);
	TextCodex::Get().DrawLine_FontFile(*pTestCanvasWorld, "あSystem123中卧槽 : Segoe UI ABC", "asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf", 30.0f, 0, 30, Colors::White);

	pTestCanvasWorld->SetRotation(0.0f, canvasAnimT * 20.0f, 0.0f);

	Test_TextBlock_ModeA_FixedCanvas(*pTestCanvasUi);
	Test_RichText_Spans(*pTestCanvasWorld);



}