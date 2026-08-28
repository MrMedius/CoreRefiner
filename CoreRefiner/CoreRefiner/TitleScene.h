#pragma once
#include "IScene.h"
#include "JsonTextCopy.h"

#include <functional>
#include <memory>

class SceneDirector;
class Window;
class UI_Title;

namespace Rgph
{
	class RenderGraph;
}

class TitleScene : public IScene
{
public:
	TitleScene(Window& wnd, SceneDirector& director, Rgph::RenderGraph& uiRG, std::function<void()> onSettings);
	~TitleScene() override;

	TitleScene(const TitleScene&) = delete;
	TitleScene& operator=(const TitleScene&) = delete;

	void OnEnter() override;
	void Update(float dt) override;
	void Submit() override;

private:
	void RefreshIfLanguageChanged_();

	Window& wnd_;
	SceneDirector& director_;
	Rgph::RenderGraph& uiRG_;
	std::unique_ptr<UI_Title> uiTitle_;
	Language language_{ Language::Zh };
};
