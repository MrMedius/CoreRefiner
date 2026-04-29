#pragma once
#include "Effect.h"
#include "RenderGraph.h"
#include "Sprite3DNoLit.h"
#include "AnimationInfo.h"
#include "Channels.h"

class Effect_Player_Hit : public Effect
{
public:
	Effect_Player_Hit(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, bool flip, int type, Object_Type_Tag tag = effect_Player_Hit)
		:
		Effect(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 8.0f,8.0f,0.0f });
		SetCollisionOnOff(false);
		pack[0] = { 5,2,1,10,20.0f };
		pack[1] = { 0,0,0, 0, 0.0f };
		pack[2] = { 3,2,1, 6,20.0f };

		// graphics init
		int t = type - 1;
		visualPre = std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{
			"asset\\Images\\\Player\\Effect_Red_Hit.png", 
			"asset\\Images\\\Player\\Player_Green_Effect.png", 
			"asset\\Images\\\Player\\Effect_Blue_Hit.png"});
		visualPre->SetFrameAuto(pack[t].numU, pack[t].numV, pack[t].FrameStart, pack[t].FrameTotalCount, t, pack[t].FPS, flip, false);
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->LinkTechniques(rg);
	}
	void SpawnWithType(XMFLOAT3 pos, bool flip, int type)
	{
		SpawnAt(pos, flip);

		int t = type - 1;
		visualPre->SetFrameAuto(pack[t].numU, pack[t].numV, pack[t].FrameStart, pack[t].FrameTotalCount, t, pack[t].FPS, flip, false);
	}
	void SpawnAt(XMFLOAT3 pos, bool flip) override
	{	
		// reset parameters
		SetPosition(pos);

		// reset animation
		visualPre->SetPosition(transInfo.position);
	}
	void OnEnable(void) override {}
	void Update(float dt) override
	{
		// anime update
		visualPre->Update(dt);

		if(visualPre->ClipFinished())
		{
			Deactivate();
		}
	}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
	}
	void OnCollide(Character* other) override {}
private:
	std::unique_ptr<Sprite3DNoLit> visualPre;
	SpriteAnimeInfo pack[3];
};