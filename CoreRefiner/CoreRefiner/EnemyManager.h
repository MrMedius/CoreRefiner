#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "Enemy.h"

class EnemyManager
{
public:
	EnemyManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EnemyManager() = default;
	void Update(float dt, bool isInCD);
	void Submit(void);
	void Reset(void);
private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Enemy*> enemies;

	float Interval{ 1.5f };
	float CountDown{ 0 };
	bool NewEnemy{ false };
	bool Created{ false };

	bool isTutorial{ true };

	Player* pPlayer;
private:
	float MapEdgeLeftAndRight{ 0.0f };
	float MapEdgeFrontAndBack{ 0.0f };
};