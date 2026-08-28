#include "TitleScene.h"

#include "GameStatsCodex.h"
#include "RenderGraph.h"
#include "SceneDirector.h"
#include "UI_Title.h"
#include "Window.h"

TitleScene::TitleScene(Window& wnd, SceneDirector& director, Rgph::RenderGraph& uiRG, std::function<void()> onSettings)
	:
	wnd_(wnd),
	director_(director),
	uiRG_(uiRG),
	uiTitle_(std::make_unique<UI_Title>(wnd.Gfx(), uiRG)),
	language_(GameStatsCodex::GetLanguage())
{
	uiTitle_->SetOnNewGame([this] { director_.RequestScene(SceneId::Game); });
	uiTitle_->SetOnSettings(std::move(onSettings));
}

TitleScene::~TitleScene() = default;

void TitleScene::OnEnter()
{
	RefreshIfLanguageChanged_();
}

void TitleScene::Update(float dt)
{
	RefreshIfLanguageChanged_();
	uiTitle_->Update(dt);
}

void TitleScene::Submit()
{
	uiTitle_->Submit();
	uiRG_.Execute(wnd_.Gfx());
}

void TitleScene::RefreshIfLanguageChanged_()
{
	const Language lang = GameStatsCodex::GetLanguage();
	if (lang == language_)
	{
		return;
	}
	language_ = lang;
	uiTitle_->RefreshLabels();
}
