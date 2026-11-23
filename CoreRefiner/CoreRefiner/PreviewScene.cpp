#include "PreviewScene.h"

#include "Ball.h"
#include "Ring_Yang.h"
#include "Ring_Yin.h"

#include "Channels.h"
#include "GameRenderGraph.h"
#include "InputCodex.h"
#include "SceneDirector.h"
#include "Window.h"

PreviewScene::PreviewScene(Window& wnd, SceneDirector& director, Rgph::GameRenderGraph& gameRG)
	:
	wnd_(wnd),
	director_(director),
	gameRG_(gameRG),
	light_(wnd.Gfx(), { 0.0f, 20.0f, 0.0f }),
	cameras_(wnd.Gfx()),
	ball_(std::make_unique<Ball>(wnd.Gfx(), gameRG, DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f }, DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f })),
	yin_(std::make_unique<Ring_Yin>(wnd.Gfx(), gameRG)),
	yang_(std::make_unique<Ring_Yang>(wnd.Gfx(), gameRG))
{
	light_.LinkTechniques(gameRG_);
	cameras_.LinkTechniques(gameRG_);
	// 灰、白、黑。只写一次，球本身不再出生。
	ball_->AppendColorId(0u);
	ball_->AppendColorId(1u);
	ball_->AppendColorId(2u);
	ball_->AppendColorId(0u);
	ball_->AppendColorId(1u);
	ball_->AppendColorId(2u);
}

PreviewScene::~PreviewScene() = default;

void PreviewScene::OnEnter()
{
	// yinPos / yangPos 是两只特效的播放中心，左右分开。
	const XMFLOAT3 yinPos{ 6.0f, 0.0f, 0.0f };
	const XMFLOAT3 yangPos{ -6.0f, 0.0f, 0.0f };
	if (yin_ != nullptr)
	{
		if (!yin_->IsActive())
		{
			yin_->Activate();
		}
		yin_->SpawnAt(yinPos, false);
	}
	if (yang_ != nullptr)
	{
		if (!yang_->IsActive())
		{
			yang_->Activate();
		}
		// hitDir 预览不需要击退，传零。Ring_Yang 只把它存下来。
		const XMFLOAT3 hitDir{ 0.0f, 0.0f, 0.0f };
		yang_->SpawnAt(yangPos, hitDir);
	}
}

void PreviewScene::OnLeave()
{
	// F2 会锁鼠标并打开原始鼠标。离开时放开，标题才能正常点。
	InputCodex::Get().DisableRawMouse();
	wnd_.EnableCursor();
}

void PreviewScene::Update(float dt)
{
	// Esc 回到标题。先判断再更新摄像机，摄像机也会读按键队列。
	if (InputCodex::Get().KeyTriggered(VK_ESCAPE))
	{
		director_.RequestScene(SceneId::Title);
	}

	// yinPos / yangPos 和进入预览时的位置相同。
	const XMFLOAT3 yinPos{ 6.0f, 0.0f, 0.0f };
	const XMFLOAT3 yangPos{ -6.0f, 0.0f, 0.0f };
	if (yin_ != nullptr)
	{
		if (!yin_->IsActive())
		{
			yin_->Activate();
			yin_->SpawnAt(yinPos, false);
		}
		yin_->Update(dt);
	}
	if (yang_ != nullptr)
	{
		if (!yang_->IsActive())
		{
			yang_->Activate();
			// hitDir 预览不需要击退，传零。Ring_Yang 只把它存下来。
			const XMFLOAT3 hitDir{ 0.0f, 0.0f, 0.0f };
			yang_->SpawnAt(yangPos, hitDir);
		}
		yang_->Update(dt);
	}

	// 跟战斗一样：主摄像机看原点，F2 操作的是窗口里选中的受控摄像机。
	cameras_.Update(dt, { 0.0f, 0.0f, 0.0f }, &wnd_);
}

void PreviewScene::Submit()
{
	light_.Bind(wnd_.Gfx(), cameras_->GetMatrix());
	gameRG_.BindMainCamera(cameras_.GetActiveCamera());

#ifdef _DEBUG
	light_.Submit(Chan::main);
#endif
	cameras_.Submit(Chan::main);
	if (ball_ != nullptr)
	{
		ball_->Submit();
	}
	if (yin_ != nullptr && yin_->IsActive())
	{
		yin_->Submit();
	}
	if (yang_ != nullptr && yang_->IsActive())
	{
		yang_->Submit();
	}
	gameRG_.Execute(wnd_.Gfx());

#ifdef _DEBUG
	// 和战斗相同的摄像机窗口。用来把画面和受控镜头都换成 Observer。
	cameras_.SpawnWindow(wnd_.Gfx());
#endif
}
