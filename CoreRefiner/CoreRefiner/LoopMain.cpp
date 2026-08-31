#include "LoopMain.h"

#include "DeferredDisableQueue.h"
#include "DisplaySettings.h"
#include "SettingsStore.h"

#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "SoundCodex.h"
#include "TimeCodex.h"

#include "GameScene.h"
#include "IScene.h"
#include "ResultScene.h"
#include "TitleScene.h"

#include "UiCopy.h"
#include "ModuleNodeInfoCopy.h"
#include "Util.h"

LoopMain::LoopMain(const std::string& commandLine)
	:
	commandLine(commandLine),
	wnd(1280, 720, "CR"),
	scriptCommander(TokenizeQuoted(commandLine))
{
	(void)LoadSettings();
	(void)LoadUiCopy();
	(void)LoadModuleNodeInfoCopy();

	ApplyWindowSizeIndex(wnd, GameStatsCodex::GetWindowSizeIndex());
	ApplyFullscreen(wnd, GameStatsCodex::GetFullscreen());

	SoundCodex::Get().PlayBGM(SndPath::BGM_Title, -1);
	SoundCodex::Get().SetMasterVolume(GameStatsCodex::GetMuted() ? 0.0f : GameStatsCodex::GetMasterVolume());
	SoundCodex::Get().SetBgmVolume(GameStatsCodex::GetBgmVolume());
	SoundCodex::Get().SetSeVolume(GameStatsCodex::GetSeVolume());
	SoundCodex::Get().SetListenerTransform(0.0f, 0.0f, 0.0f, 0, 0, 1, 0, 1, 0);

	AssembleScenes_();
}

void LoopMain::AssembleScenes_()
{
	director_ = std::make_unique<SceneDirector>();
	const auto onSettings = [this] { OpenSettings_(); };

	director_->Adopt(SceneId::Title, std::make_unique<TitleScene>(wnd, *director_, uiRG, onSettings));
	director_->Adopt(SceneId::Game, std::make_unique<GameScene>(wnd, *director_, gameRG, onSettings));
	director_->Adopt(SceneId::Result, std::make_unique<ResultScene>(wnd, uiRG));

	director_->RequestScene(SceneId::Title);

	uiSetting_ = std::make_unique<UI_Setting>(wnd.Gfx(), uiRG);
	uiSetting_->SetOnBack([this] { CloseSettings_(); });
	uiSetting_->SetOnFullscreenChanged([this](bool on) { ApplyFullscreen(wnd, on); });
	uiSetting_->SetOnWindowSizeIndex([this](int index) { ApplyWindowSizeIndex(wnd, index); });
}

LoopMain::~LoopMain()
{
	(void)SaveSettings();
}

int LoopMain::Run()
{
	while (true)
	{
		if (const auto ecode = Window::ProcessMessages())
		{
			return *ecode;
		}

		if (timer_update.Peek() >= 1.0f / 60.0f)
		{
#ifdef _DEBUG
			FrameCounter++;
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
			wnd.Gfx().EndFrame();
			gameRG.Reset();
			uiRG.Reset();
			/********************************/
		}
	}
}

bool LoopMain::IsSettingsOpen_() const noexcept
{
	return uiSetting_ != nullptr && uiSetting_->IsOpen();
}

void LoopMain::OpenSettings_()
{
	if (uiSetting_ == nullptr)
	{
		return;
	}
	wnd.EnableCursor();
	uiSetting_->Show();
	InputCodex::Get().FlushKeyboard();
}

void LoopMain::CloseSettings_()
{
	if (!IsSettingsOpen_())
	{
		return;
	}
	uiSetting_->Hide();
	(void)SaveSettings();
	InputCodex::Get().FlushKeyboard();
}

void LoopMain::Update(float dt)
{
	if (IsSettingsOpen_())
	{
		if (InputCodex::Get().KeyTriggered(VK_ESCAPE))
		{
			CloseSettings_();
		}
		if (IsSettingsOpen_())
		{
			uiSetting_->Update(dt);
		}
		DeferredDisableQueue::Get().Flush();
		return;
	}

#ifdef _DEBUG
	if (InputCodex::Get().KeyTriggered(VK_SHIFT) && director_ != nullptr)
	{
		const SceneId cur = director_->GetCurrent();

		if (cur < SceneId::Count)
		{
			const auto next = static_cast<SceneId>((static_cast<unsigned char>(cur) + 1u) % static_cast<unsigned char>(SceneId::Count));
			director_->RequestScene(next);
		}
	}
#endif

	if (director_ != nullptr)
	{
		director_->Update(dt);
	}
	DeferredDisableQueue::Get().Flush();
}

void LoopMain::Draw()
{
	if (IsSettingsOpen_())
	{
		uiSetting_->Submit();
		uiRG.Execute(wnd.Gfx());
		return;
	}

	if (director_ != nullptr)
	{
		director_->Submit();
	}
}
