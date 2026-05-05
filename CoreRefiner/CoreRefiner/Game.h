#pragma once
#include "Window.h"
#include "Timer.h"
#include "ImguiManager.h"
#include "CameraContainer.h"
#include "PointLight.h"
#include "ScriptCommander.h"

#include "InGameRenderGraph.h"
#include "InUserInterfaceRenderGraph.h"

#include "Player.h"
#include "AttackManager.h"
#include "EnvironmentManager.h"
#include "EnemyManager.h"

#include "Canvas2D.h"
#include "Canvas3D.h"

class Game
{
private:
	enum SCENE : int
	{
		SCENE_TITLE,
		SCENE_GAME,
		SCENE_RESULT,
	}Scene{ SCENE_TITLE };
public:
	Game( const std::string& commandLine = "" );
	int RunGame();
	~Game();
private:
	void Update(float dt);
	void Draw(void);
	void SetScene(SCENE scene) { Scene = scene; }
	/********************************/
	/*         Game Related         */
	/********************************/
private:
	// Core components
	std::string commandLine;
	ImguiManager imgui;
	Window wnd;
	ScriptCommander scriptCommander;
	bool Pause{ false };
	// FPS calculation
	std::chrono::steady_clock::time_point LastFrameTime;
	std::chrono::steady_clock::time_point StartFrameTime;
	int FrameCounter{ 0 };
	Timer timer_update;
	Timer timer_FPS;
	float speed_factor = 1.0f;
	/********************************/
	/*         Game Related         */
	/********************************/
	PointLight light;
	CameraContainer cameras{ wnd.Gfx() };
	Rgph::InGameRenderGraph gameRG{ wnd.Gfx() };
	Rgph::InUserInterfaceRenderGraph UIRG{ wnd.Gfx() };
	// Objects
	Player* pPlayer;
	std::unique_ptr<AttackManager> pAttackManager;
	std::unique_ptr<EnvironmentManager> pEnvironmentManager;
	std::unique_ptr<EnemyManager> pEnemyManager;

	std::unique_ptr<Canvas2D> pTestCanvasUi;
	std::unique_ptr<Canvas3D> pTestCanvasWorld;
	void InitCanvasDemo();
	void UpdateCanvasDemo(float dt);
	float canvasAnimT = 0.0f;
};