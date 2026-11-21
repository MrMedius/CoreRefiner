#include "EffectManager.h"

#include "Ring_Yin.h"
#include "HierarchySpawn.h"
#include "ObjectCodex.h"

EffectManager::EffectManager(Graphics& gfx, Rgph::RenderGraph& rg)
	:
	gfx_(gfx),
	rg_(rg)
{
	// 先生成再关掉，池里留着不播放的实例。
	HierarchySpawn::WarmupPool<Ring_Yin>(effect_Ring_Yin, kPoolCount_, gfx_, rg_);
}

void EffectManager::Update(float dt)
{
	for (Ring_Yin* fx : ObjectCodex::FindActiveObjectsByTag<Ring_Yin>(effect_Ring_Yin))
	{
		if (fx != nullptr)
		{
			fx->Update(dt);
		}
	}
}

void EffectManager::Submit()
{
	for (Ring_Yin* fx : ObjectCodex::FindActiveObjectsByTag<Ring_Yin>(effect_Ring_Yin))
	{
		if (fx != nullptr)
		{
			fx->Submit();
		}
	}
}

void EffectManager::Reset()
{
	for (Ring_Yin* fx : ObjectCodex::FindObjectsByTag<Ring_Yin>(effect_Ring_Yin))
	{
		if (fx != nullptr && fx->IsActive())
		{
			fx->Deactivate();
		}
	}
}
