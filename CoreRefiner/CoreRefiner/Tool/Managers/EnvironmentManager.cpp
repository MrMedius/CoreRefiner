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
	// 场地自己放进名单，不交接。
	environments.push_back(ObjectCodex::AcquirePersistent<Field>(environment_Field, gfx, rg, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 100.0f, 4.0f, 100.0f }, true));
	// 先生成再关掉。没交出，所以不会进 environments。
	for (int i = 0;i < 10;i++)
	{
		ObjectCodex::SpawnPooled<Coin>(environment_Coin, gfx, rg, XMFLOAT3{ 0.0f,0.0f,0.0f });
	}
	for (int i = 0;i < 10;i++)
	{
		ObjectCodex::FindFirstActiveObjectByTag<Coin>(environment_Coin)->Deactivate();
	}
}

void EnvironmentManager::ClaimEnvironment()
{
	for (EnvironmentBase* env : ObjectCodex::TakeHandoff<EnvironmentBase>())
	{
		if (env == nullptr || !env->IsActive())
		{
			continue;
		}
		bool tracked = false;
		for (EnvironmentBase* existing : environments)
		{
			if (existing == env)
			{
				tracked = true;
				break;
			}
		}
		if (!tracked)
		{
			environments.push_back(env);
		}
	}
}

void EnvironmentManager::Update(float dt)
{
	// 金币在敌人更新里掉落，而这一步在敌人之前，所以当帧掉的下一帧才进名单。
	ClaimEnvironment();
	for (int i = 0; i < static_cast<int>(environments.size()); ++i)
	{
		EnvironmentBase* env = environments[static_cast<std::size_t>(i)];
		if (env != nullptr && env->IsActive())
		{
			env->Update(dt);
		}
		else
		{
			environments.erase(environments.begin() + i);
			--i;
		}
	}
}

void EnvironmentManager::Submit(void)
{
	for (EnvironmentBase* env : environments)
	{
		if (env != nullptr && env->IsActive())
		{
			env->Submit();
		}
	}
}

void EnvironmentManager::Reset(void)
{
	for (EnvironmentBase* env : environments)
	{
		if (env != nullptr && env->IsActive())
		{
			env->Deactivate();
		}
	}
	environments.clear();
	for (EnvironmentBase* env : ObjectCodex::TakeHandoff<EnvironmentBase>())
	{
		if (env != nullptr && env->IsActive())
		{
			env->Deactivate();
		}
	}
	isLearnt = false;
	isTutorial = true;
}

void EnvironmentManager::EnterGame(void)
{
	if (!environments.empty())
	{
		for (EnvironmentBase* env : environments)
		{
			if (env != nullptr)
			{
				env->Activate();
			}
		}
		return;
	}
	// 离开游戏时名单已清空。场地还在对象池里，找回来再启动。
	if (Field* field = ObjectCodex::FindFirstObjectByTag<Field>(environment_Field))
	{
		field->Activate();
		environments.push_back(field);
	}
}

void EnvironmentManager::BeginVacuum()
{
	// 清场发生在本帧环境更新之前。先认领，上一帧交出的金币才会飞向玩家。
	ClaimEnvironment();
	for (EnvironmentBase* env : environments)
	{
		// 强制吸附是金币自己的行为。
		if (Coin* coin = dynamic_cast<Coin*>(env); coin != nullptr && coin->IsActive())
		{
			coin->SetForceMagnet(true);
		}
	}
}

bool EnvironmentManager::HasActiveCoins() const
{
	for (EnvironmentBase* env : environments)
	{
		if (Coin* coin = dynamic_cast<Coin*>(env); coin != nullptr && coin->IsActive())
		{
			return true;
		}
	}
	return false;
}

void EnvironmentManager::CollectAllCoins()
{
	ClaimEnvironment();
	for (EnvironmentBase* env : environments)
	{
		// 立刻收走是金币自己的行为。
		if (Coin* coin = dynamic_cast<Coin*>(env); coin != nullptr && coin->IsActive())
		{
			coin->CollectNow();
		}
	}
}