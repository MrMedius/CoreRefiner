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

#include "ModuleWorkbench.h"
#include "UI_Title.h"
#include "UI_Prep.h"
#include "UI_Pause.h"
#include "UI_CombatHud.h"
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
	void UpdateGameScene_(float dt);
	void UpdateCombat_(float dt);
	void UpdateVacuum_(float dt);
	void UpdatePrep_(float dt);
	void SyncCombatHud_();


	void SetScene(SCENE scene);
	void LeaveScene(SCENE scene);
	void EnterScene(SCENE scene);

	void EnterCombatPresentation_();
	void EnterPrepPresentation_();
	void BeginVacuumSweep_();
	void TryFinishVacuum_(float dt);
	void TryNotifyPlayerDead_();
	void TryEnterResultIfTerminal_();
	[[nodiscard]] bool IsCombatWorld_() const noexcept;
	void ClosePauseMenu_() noexcept;
	void TryTogglePauseMenu_() noexcept;

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

	std::unique_ptr<ModuleWorkbench> moduleWorkbench;
	std::unique_ptr<UI_Title> uiTitle;
	std::unique_ptr<UI_Prep> uiPrep;
	std::unique_ptr<UI_Pause> uiPause;
	std::unique_ptr<UI_CombatHud> uiCombatHud;
	std::unique_ptr<UI_Sample> uiSample;
};
