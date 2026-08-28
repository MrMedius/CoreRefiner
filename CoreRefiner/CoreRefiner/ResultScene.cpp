#include "ResultScene.h"

#include "RenderGraph.h"
#include "UI_Sample.h"
#include "Window.h"

ResultScene::ResultScene(Window& wnd, Rgph::RenderGraph& uiRG)
	:
	wnd_(wnd),
	uiRG_(uiRG),
	uiSample_(std::make_unique<UI_Sample>(wnd.Gfx(), uiRG))
{
}

ResultScene::~ResultScene() = default;

void ResultScene::Update(float dt)
{
	uiSample_->Update(dt);
}

void ResultScene::Submit()
{
	uiSample_->Submit();
	uiRG_.Execute(wnd_.Gfx());
}
