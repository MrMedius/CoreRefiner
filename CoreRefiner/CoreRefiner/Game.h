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

#include "UI_Title.h"
#include "UI_Game.h"
#include "UI_Sample.h"
#include "WaveDirector.h"

class Game
{
private:
	enum SCENE : int
	{
		SCENE_TITLE,
		SCENE_GAME,
		SCENE_RESULT,
		SCENE_COUNT
	}Scene{ SCENE_TITLE };
public:
	Game(const std::string& commandLine = "");
	int RunGame();
	~Game();
private:
	void Update(float dt);
	void Draw(void);
	/**
	 * @brief Switch scene with leave/enter discipline (does nothing if same scene).
	 */
	void SetScene(SCENE scene);
	void LeaveScene(SCENE scene);
	void EnterScene(SCENE scene);
	void EnterCombatPresentation_();
	void EnterPrepPresentation_();
	/** @brief 停刷怪、清敌人与弹幕，并让残留金币强制飞向玩家。 */
	void BeginVacuumSweep_();
	/** @brief 金币吸完或超时后 FinishWave。 */
	void TryFinishVacuum_(float dt);
	/** @brief Combat / Vacuum 都要画世界。 */
	[[nodiscard]] bool IsCombatWorld_() const noexcept;
	/********************************/
	/*         Game Related         */
	/********************************/
private:
	// Core components
	std::string commandLine;
	ImguiManager imgui;
	Window wnd;
	ScriptCommander scriptCommander;
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
	// Renender
	PointLight light;
	CameraContainer cameras{ wnd.Gfx() };
	Rgph::InGameRenderGraph gameRG{ wnd.Gfx() };
	Rgph::InUserInterfaceRenderGraph UIRG{ wnd.Gfx() };
	// GameArramgement
	bool Pause{ false };
	WaveDirector waveDirector_;
	float vacuumElapsed_{ 0.0f };
	static constexpr float kVacuumTimeout_{ 3.0f };
	// Objects
	Player* pPlayer;
	std::unique_ptr<AttackManager> pAttackManager;
	std::unique_ptr<EnvironmentManager> pEnvironmentManager;
	std::unique_ptr<EnemyManager> pEnemyManager;

	std::unique_ptr<UI_Title> uiTitle;
	std::unique_ptr<UI_Game> uiGame;
	std::unique_ptr<UI_Sample> uiSample;
};
