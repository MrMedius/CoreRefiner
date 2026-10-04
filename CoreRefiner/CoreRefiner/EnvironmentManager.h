#pragma once
#include "Graphics.h"
#include "RenderGraph.h"

#include "Environment.h"

class EnvironmentManager
{
public:
	EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EnvironmentManager() = default;
	void Update(float dt);
	void Submit(void);
	// Leave GAME: deactivate environments (keep pointers; no ClearTag).
	void Reset(void);
	// Enter GAME: re-Activate all environments.
	void EnterGame(void);
	// 场上所有活跃金币开始强制飞向玩家。
	void BeginVacuum();
	// 场上是否还有未吸收的金币。
	[[nodiscard]] bool HasActiveCoins() const;
	// 立刻结算残留金币（吸取超时兜底）。
	void CollectAllCoins();
private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Environment*> eG;

	bool isLearnt{ false };
	bool isTutorial{ true };
};