#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "Skybox.h"
#include "Channels.h"
#include "Player.h"

class SkyboxObj : public Environment
{
public:
	SkyboxObj(Graphics& gfx, Rgph::RenderGraph& rg, Object_Type_Tag tag = environment_Skybox)
		:
		Environment(tag)
	{
		pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

		// graphics init
		visualPre = std::make_unique<Skybox>(gfx, std::vector<std::string>{ 
				"asset\\Images\\Skybox\\Usual",
				"asset\\Images\\Skybox\\Red",
				"asset\\Images\\Skybox\\Green",
				"asset\\Images\\Skybox\\Blue"
		});
		visualPre->LinkTechniques(rg);
	}
	void OnEnable(void) override 
	{
		visualPre->ResetMode();
	}
	void Update(float dt) override
	{
		if (!fieldPlay)
		{
			if (pPlayer->GetIsSkill())
			{
				fieldPlay = true;

				switch (pPlayer->GetWeaponSlots()->GetCurrentSlot().GetType())
				{
				case WEAPON_TYPE_1: fieldMode = 1; break;
				case WEAPON_TYPE_2: fieldMode = 2; break;
				case WEAPON_TYPE_3: fieldMode = 3; break;
				}

				if (visualPre->GetPlayMode() != fieldMode)
					visualPre->StartExpand(fieldMode);
			}
		}

		if (!pPlayer->GetIsSkill() && fieldPlay)
		{
			fieldPlay = false;
			fieldMode = 0;
		}

		visualPre->Update(dt);
	}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
	}
	void OnCollide(Character* other) override {}
private:
	std::unique_ptr<Skybox> visualPre;
	Player* pPlayer;
	int fieldMode{ 0 };
	bool fieldPlay{ false };
};