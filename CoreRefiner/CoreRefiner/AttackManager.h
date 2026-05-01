#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "Attack.h"

class AttackManager
{
public:
	AttackManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~AttackManager() = default;
	void Update(float dt);
	void Submit(void);
	void Reset(void);
private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Attack*> attacks;
	int playerRemote{ 0 };

	Player* pPlayer;
};

