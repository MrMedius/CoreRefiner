#include "EnvironmentManager.h"

#include "GameStatsCodex.h"
#include "ObjectCodex.h"

#include "SkyboxObj.h"
#include "Block.h"
#include "BlockTiled.h"
#include "BlockInvisible.h"
#include "BlockFog.h"
#include "FireflyField.h"
#include "SkyGridField.h"
#include "BlockTiledTutorial.h"

EnvironmentManager::EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	// Environments Tutorial
	{
		// BlockTiled
		XMFLOAT3 fieldSize = XMFLOAT3(50.0f, 2.0f, 50.0f);
		eT.push_back(ObjectCodex::Acquire<BlockTiledTutorial>(environment_BlockTiledTutorial, gfx, rg, XMFLOAT3(0.0f, 40.0f, 0.0f), fieldSize, XMFLOAT2(2.0f, 2.0f), true));		
		// BlockInvisible
		float longSide = 100.0f, shortSide = 50.0f;
		eT.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ fieldSize.x / 2.0f + shortSide / 2.0f, 10.0f, 0.0f }, XMFLOAT3{ shortSide, 100.0f, longSide }, true));
		eT.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ -fieldSize.x / 2.0f - shortSide / 2.0f, 10.0f, 0.0f }, XMFLOAT3{ shortSide, 100.0f, longSide }, true));
		eT.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ 0.0f, 10.0f,  fieldSize.z / 2.0f + shortSide / 2.0f }, XMFLOAT3{ longSide, 100.0f, shortSide }, true));
		eT.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ 0.0f, 10.0f, -fieldSize.z / 2.0f - shortSide / 2.0f }, XMFLOAT3{ longSide, 100.0f, shortSide }, true));
		eT.push_back(ObjectCodex::Acquire<SkyGridField>(environment_SkyGridField, gfx, rg));
	}
	// Environments Game
	{
		// Skybox
		eG.push_back(ObjectCodex::Acquire<SkyboxObj>(environment_Skybox, gfx, rg));
		// Block
		//eG.push_back(ObjectCodex::Acquire<Block>(environment_Block, gfx, rg, XMFLOAT3{ 4.0f, 3.0f, 4.0f }, XMFLOAT3{ 4.0f, 4.0f, 4.0f }, true));
		// BlockTiled
		XMFLOAT3 fieldSize = XMFLOAT3(300.0f, 2.0f, 100.0f);
		eG.push_back(ObjectCodex::Acquire<BlockTiled>(environment_BlockTiled, gfx, rg, XMFLOAT3(0.0f, 0.0f, 0.0f), fieldSize, XMFLOAT2(30.0f, 10.0f), true));
		// BlockInvisible
		float longSide = 300.0f, shortSide = 50.0f;
		eG.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ fieldSize.x / 2.0f + shortSide / 2.0f, 0.0f, 0.0f }, XMFLOAT3{ shortSide, 100.0f, longSide }, true));
		eG.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ -fieldSize.x / 2.0f - shortSide / 2.0f, 0.0f, 0.0f }, XMFLOAT3{ shortSide, 100.0f, longSide }, true));
		eG.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ 0.0f, 0.0f,  fieldSize.z / 2.0f + shortSide / 2.0f }, XMFLOAT3{ longSide, 100.0f, shortSide }, true));
		eG.push_back(ObjectCodex::Acquire<BlockInvisible>(environment_BlockInvisible, gfx, rg,
			XMFLOAT3{ 0.0f, 0.0f, -fieldSize.z / 2.0f - shortSide / 2.0f }, XMFLOAT3{ longSide, 100.0f, shortSide }, true));
		// BlockFog
		eG.push_back(ObjectCodex::Acquire<BlockFog>(environment_BlockFog, gfx, rg, XMFLOAT3(0.0f, -300.0f, 0.0f), XMFLOAT3(3000.0f, 500.0f, 3000.0f)));
		// FireflyField
		eG.push_back(ObjectCodex::Acquire<FireflyField>(environment_FireflyField, gfx, rg));
	}
	// Reset
	Reset();
}

void EnvironmentManager::Update(float dt)
{
	if (isTutorial)
	{
		for (int i = 0;i < eT.size();i++)
		{
			eT[i]->Update(dt);
		}
	}
	if (!isTutorial || GameStatsCodex::GetIsLearnt())
	{
		for (int i = 0;i < eG.size();i++)
		{
			eG[i]->Update(dt);
		}
	}


	if (!isLearnt && GameStatsCodex::GetIsLearnt())
	{
		size_t size = 0;
		for (size_t i = 0;i < eT.size();i++)
		{
			if (eT[i]->IsActive()) 
				size++;
		}
		if (eT.size() - size >= 1)
		{
			isLearnt = true;
			GameStatsCodex::SetFinishPart();
			for (int i = 0;i < eG.size();i++) eG[i]->Activate();
		}
	}

	if (isTutorial && isTutorial != GameStatsCodex::GetIsTutorial())
	{
		isTutorial = GameStatsCodex::GetIsTutorial();
		for (int i = 0;i < eT.size();i++) eT[i]->Deactivate();
	}
}

void EnvironmentManager::Submit(void)
{
	if (isTutorial)
	{
		for (int i = 0;i < eT.size();i++)
		{
			eT[i]->Submit();
		}
	}
	if (!isTutorial || GameStatsCodex::GetIsLearnt())
	{
		for (int i = 0;i < eG.size();i++)
		{
			eG[i]->Submit();
		}
	}
}

void EnvironmentManager::Reset(void)
{
	for (int i = 0;i < eT.size();i++) eT[i]->Activate();
	for (int i = 0;i < eG.size();i++) eG[i]->Deactivate();
	isLearnt = false;
	isTutorial = true;
}

void EnvironmentManager::SpawnWindow()
{
	for (int i = 0;i < eG.size();i++)
	{
		if (auto* a = dynamic_cast<FireflyField*>(eG[i]))
		{
			a->SpawnWindow();
		}
	}
}