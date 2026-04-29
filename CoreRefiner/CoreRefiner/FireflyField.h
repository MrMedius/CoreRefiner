#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "FireflyEffect.h"
#include "Channels.h"
#include "Player.h"

class FireflyField : public Environment
{
public:
	FireflyField(Graphics& gfx, Rgph::RenderGraph& rg, Object_Type_Tag tag = environment_FireflyField)
		:
		Environment(tag)
	{
		pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

		// parameters init
		SetCollisionOnOff(false);

		// graphics init
		visualPre = std::make_unique<FireflyEffect>(gfx);
		visualPre->SetColor(base);
		visualPre->LinkTechniques(rg);
	}
	void OnEnable(void) override 
	{
		input = base;
	}
	void Update(float dt) override
	{
		auto playerPos = pPlayer->GetPosition();

		if (pPlayer->GetIsSkill())
		{
			switch (pPlayer->GetWeaponSlots()->GetCurrentSlot().GetType())
			{
			case WEAPON_TYPE_1: input.x += 0.3f; break;
			case WEAPON_TYPE_2: input.y += 0.3f; break;
			case WEAPON_TYPE_3: input.z += 0.3f; break;
			}

			input.x = std::clamp(input.x, 0.1f, 1.0f);
			input.y = std::clamp(input.y, 0.1f, 1.0f);
			input.z = std::clamp(input.z, 0.1f, 1.0f);

			visualPre->SetColor(input);
		}
		else if (input.x > base.x || input.y > base.y || input.z > base.z)
		{
			float offset = dt * 0.5f;
			input = { input.x - offset,input.y - offset ,input.z - offset,1.0f };

			input.x = std::clamp(input.x, 0.1f, 1.0f);
			input.y = std::clamp(input.y, 0.1f, 1.0f);
			input.z = std::clamp(input.z, 0.1f, 1.0f);

			visualPre->SetColor(input);
		}

		visualPre->Update(dt);
	}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
	}
	void OnCollide(Character* other) override {}

	void SpawnWindow(void) const
	{
		visualPre->SpawnWindow();
	}
private:
	std::unique_ptr<FireflyEffect> visualPre;
	Player* pPlayer;
	static constexpr XMFLOAT4 base = { 0.1f,0.1f,0.1f,0.5f };
	XMFLOAT4 input{ base };
};