#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "Effect.h"

class EffectManager
{
public:
	EffectManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EffectManager() = default;
	void Update(float dt);
	void Submit(void);
	void Reset(void);
private:
	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Effect*> effects;
	int playerRemote{ 0 };

	Player* pPlayer;
	Attack_Type_Tag playerAttackType{ Attack_Type_None };
};