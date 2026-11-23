#pragma once
#include "Graphics.h"
#include "RenderGraph.h"

#include "EnvironmentBase.h"

class EnvironmentManager
{
public:
	EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EnvironmentManager() = default;
	void Update(float dt);
	void Submit(void);
	// 离开游戏：关掉还开着的环境物体并清空名单。对象仍留在对象池里。
	void Reset(void);
	// 进入游戏：名单空时从对象池找回场地，放进来并启动。
	void EnterGame(void);
	// 场上所有活跃金币开始强制飞向玩家。
	void BeginVacuum();
	// 场上是否还有未吸收的金币。
	[[nodiscard]] bool HasActiveCoins() const;
	// 立刻结算残留金币（吸取超时兜底）。
	void CollectAllCoins();
private:
	// 取走所有待认领的 EnvironmentBase，放进 environments。已经停用或已经在名单里的不放。
	void ClaimEnvironment();

	Graphics& gfx;
	Rgph::RenderGraph& rg;

	// 正在使用的环境物体。场地在构造或进游戏时放进来，认领来的接在后面。
	std::vector<EnvironmentBase*> environments;

	bool isLearnt{ false };
	bool isTutorial{ true };
};