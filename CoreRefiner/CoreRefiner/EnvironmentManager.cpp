#include "EnvironmentManager.h"

#include "GameStatsCodex.h"
#include "ObjectCodex.h"

#include "Field.h"


EnvironmentManager::EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	// Environments Game
	{
		// Field
		eG.push_back(ObjectCodex::Acquire<Field>(environment_Field, gfx, rg, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 50.0f, 4.0f, 50.0f }, true));
	}
	for (int i = 0;i < eG.size();i++) 
		eG[i]->Activate();
}

void EnvironmentManager::Update(float dt)
{
	for (int i = 0;i < eG.size();i++)
	{
		eG[i]->Update(dt);
	}
}

void EnvironmentManager::Submit(void)
{
	for (int i = 0;i < eG.size();i++)
	{
		eG[i]->Submit();
	}
}

void EnvironmentManager::Reset(void)
{
	for (int i = 0;i < eG.size();i++) eG[i]->Deactivate();
	isLearnt = false;
	isTutorial = true;
}