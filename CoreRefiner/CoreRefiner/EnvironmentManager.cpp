#include "EnvironmentManager.h"

#include "GameStatsCodex.h"
#include "ObjectCodex.h"

#include "Field.h"
#include "Coin.h"


EnvironmentManager::EnvironmentManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx(gfx),
	rg(rg)
{
	// Environments Game
	{
		// Field
		eG.push_back(ObjectCodex::AcquirePersistent<Field>(environment_Field, gfx, rg, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 100.0f, 4.0f, 100.0f }, true));
		// Coin
		for (int i = 0;i < 10;i++)
		{
			ObjectCodex::SpawnPooled<Coin>(environment_Coin, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
		}
		for (int i = 0;i < 10;i++)
		{
			ObjectCodex::FindFirstActiveObjectByTag<Coin>(environment_Coin)->Deactivate();
		}
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

	for (Coin* coin : ObjectCodex::FindActiveObjectsByTag<Coin>(environment_Coin))
	{
		if (coin != nullptr)
		{
			coin->Update(dt);
		}
	}
}

void EnvironmentManager::Submit(void)
{
	for (int i = 0;i < eG.size();i++)
	{
		eG[i]->Submit();
	}

	for (Coin* coin : ObjectCodex::FindActiveObjectsByTag<Coin>(environment_Coin))
	{
		if (coin != nullptr)
		{
			coin->Submit();
		}
	}
}

void EnvironmentManager::Reset(void)
{
	for (int i = 0; i < eG.size(); i++) eG[i]->Deactivate();
	for (Coin* coin : ObjectCodex::FindObjectsByTag<Coin>(environment_Coin))
	{
		if (coin != nullptr && coin->IsActive())
		{
			coin->Deactivate();
		}
	}
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

void EnvironmentManager::BeginVacuum()
{
	for (Coin* coin : ObjectCodex::FindActiveObjectsByTag<Coin>(environment_Coin))
	{
		if (coin != nullptr)
		{
			coin->SetForceMagnet(true);
		}
	}
}

bool EnvironmentManager::HasActiveCoins() const
{
	for (Coin* coin : ObjectCodex::FindActiveObjectsByTag<Coin>(environment_Coin))
	{
		if (coin != nullptr)
		{
			return true;
		}
	}
	return false;
}

void EnvironmentManager::CollectAllCoins()
{
	for (Coin* coin : ObjectCodex::FindActiveObjectsByTag<Coin>(environment_Coin))
	{
		if (coin != nullptr)
		{
			coin->CollectNow();
		}
	}
}