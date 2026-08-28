#pragma once
#include "Window.h"
#include "Timer.h"
#include "ImguiManager.h"
#include "ScriptCommander.h"

#include "GameRenderGraph.h"
#include "UserInterfaceRenderGraph.h"
#include "SceneDirector.h"
#include "UI_Setting.h"

#include <memory>
#include <string>

class LoopMain
{
public:
	LoopMain(const std::string& commandLine = "");
	int Run();
	~LoopMain();

private:
	void Update(float dt);
	void Draw();

	void AssembleScenes_();

	void OpenSettings_();
	void CloseSettings_();
	[[nodiscard]] bool IsSettingsOpen_() const noexcept;

	std::string commandLine;
	ImguiManager imgui;
	Window wnd;
	ScriptCommander scriptCommander;
	int FrameCounter{ 0 };
	Timer timer_update;
	Timer timer_FPS;
	float speed_factor = 1.0f;

	Rgph::GameRenderGraph gameRG{ wnd.Gfx() };
	Rgph::UserInterfaceRenderGraph uiRG{ wnd.Gfx() };

	std::unique_ptr<SceneDirector> director_;
	std::unique_ptr<UI_Setting> uiSetting_;
};
