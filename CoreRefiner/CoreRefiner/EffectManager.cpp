#include "EffectManager.h"

#include "EffectBase.h"
#include "RingYin.h"
#include "ExecuteSphere.h"
#include "HierarchySpawn.h"
#include "ObjectCodex.h"

EffectManager::EffectManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx_(gfx),
	rg_(rg)
{
	// 先生成再关掉。没交出，所以不会进 effects。阴环和黑球各预热一份。
	HierarchySpawn::WarmupPool<RingYin>(effect_Ring_Yin, kPoolCount_, gfx_, rg_);
	HierarchySpawn::WarmupPool<ExecuteSphere>(effect_ExecuteSphere, kPoolCount_, gfx_, rg_);
}

void EffectManager::ClaimEffect()
{
	for (EffectBase* fx : ObjectCodex::TakeHandoff<EffectBase>())
	{
		if (fx == nullptr || !fx->IsActive())
		{
			continue;
		}
		bool tracked = false;
		for (EffectBase* existing : effects)
		{
			if (existing == fx)
			{
				tracked = true;
				break;
			}
		}
		if (!tracked)
		{
			effects.push_back(fx);
		}
	}
}

void EffectManager::Update(float dt)
{
	// 特效在攻击更新里生成。特效更新在那之后，所以当帧就能播。
	ClaimEffect();
	for (int i = 0; i < static_cast<int>(effects.size()); ++i)
	{
		EffectBase* fx = effects[i];
		if (fx != nullptr && fx->IsActive())
		{
			fx->Update(dt);
		}
		else
		{
			effects.erase(effects.begin() + i);
			--i;
		}
	}
}

void EffectManager::Submit()
{
	for (EffectBase* fx : effects)
	{
		if (fx != nullptr && fx->IsActive())
		{
			fx->Submit();
		}
	}
}

void EffectManager::Reset()
{
	for (EffectBase* fx : effects)
	{
		if (fx != nullptr && fx->IsActive())
		{
			fx->Deactivate();
		}
	}
	effects.clear();
	for (EffectBase* fx : ObjectCodex::TakeHandoff<EffectBase>())
	{
		if (fx != nullptr && fx->IsActive())
		{
			fx->Deactivate();
		}
	}
}
