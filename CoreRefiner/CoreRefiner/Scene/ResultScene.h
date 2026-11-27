#pragma once

#include "IScene.h"

#include <memory>

class Window;
class UI_Sample;

namespace Rgph
{
	class RenderGraph;
}

class ResultScene : public IScene
{
public:
	ResultScene(Window& wnd, Rgph::RenderGraph& uiRG);
	~ResultScene() override;

	ResultScene(const ResultScene&) = delete;
	ResultScene& operator=(const ResultScene&) = delete;

	void Update(float dt) override;
	void Submit() override;

private:
	Window& wnd_;
	Rgph::RenderGraph& uiRG_;
	std::unique_ptr<UI_Sample> uiSample_;
};
