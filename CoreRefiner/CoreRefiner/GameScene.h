#pragma once
#include "IScene.h"
#include "CameraContainer.h"
#include "JsonTextCopy.h"
#include "PointLight.h"
#include "WaveDirector.h"

#include <functional>
#include <memory>

class SceneDirector;
class Window;
class Player;
class AttackManager;
class EnvironmentManager;
class EnemyManager;
class ModuleWorkbench;
class UI_Prep;
class UI_Pause;
class UI_CombatHud;

namespace Rgph
{
	class GameRenderGraph;
}

class GameScene : public IScene
{
public:
	GameScene(Window& wnd, SceneDirector& director, Rgph::GameRenderGraph& gameRG, std::function<void()> onSettings);
	~GameScene() override;

	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;

	void OnEnter() override;
	void OnLeave() override;
	void Update(float dt) override;
	void Submit() override;

private:
	void EnterCombatPresentation_();
	void EnterPrepPresentation_();

	void BeginVacuumSweep_();
	void TryFinishVacuum_(float dt);
	
	void TryNotifyPlayerDead_();
	void TryEnterResultIfTerminal_();
	
	[[nodiscard]] bool IsCombatWorld_() const noexcept;
	
	void ClosePauseMenu_() noexcept;
	void TryTogglePauseMenu_() noexcept;
	
	void RefreshIfLanguageChanged_();
	
	void SyncCombatHud_();
	
	void UpdateCombat_(float dt);
	void UpdateVacuum_(float dt);
	void UpdatePrep_(float dt);

	Window& wnd_;
	SceneDirector& director_;
	Rgph::GameRenderGraph& gameRG_;
	std::function<void()> onSettings_;

	PointLight light_;
	CameraContainer cameras_;

	bool pause_{ false };
	Language language_{ Language::Zh };

	WaveDirector waveDirector_;
	float vacuumElapsed_{ 0.0f };
	static constexpr float kVacuumTimeout_{ 3.0f };

	Player* pPlayer_{ nullptr };

	std::unique_ptr<AttackManager> pAttackManager_;
	std::unique_ptr<EnvironmentManager> pEnvironmentManager_;
	std::unique_ptr<EnemyManager> pEnemyManager_;

	std::unique_ptr<ModuleWorkbench> moduleWorkbench_;

	std::unique_ptr<UI_Prep> uiPrep_;
	std::unique_ptr<UI_Pause> uiPause_;
	std::unique_ptr<UI_CombatHud> uiCombatHud_;
};
