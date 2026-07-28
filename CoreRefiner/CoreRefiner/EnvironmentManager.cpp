#include "EnvironmentManager.h"

#include "GameStatsCodex.h"
#include "ObjectCodex.h"

#include "Field.h"
#ifdef _DEBUG
#include "CapsuleProbe.h"
#endif


EnvironmentManager::EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	// Environments Game
	{
		// Field
		eG.push_back(ObjectCodex::AcquirePersistent<Field>(environment_Field, gfx, rg, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 100.0f, 4.0f, 100.0f }, true));
#ifdef _DEBUG
		// Static capsule probe (cyan wire) for Sphere/Box overlap tests — at (5,2,5)
		eG.push_back(ObjectCodex::AcquirePersistent<CapsuleProbe>(environment_CapsuleProbe, gfx, rg));
#endif
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

void EnvironmentManager::EnterGame(void)
{
	for (int i = 0; i < eG.size(); i++)
	{
		if (eG[i] != nullptr)
		{
			eG[i]->Activate();
		}
	}
}