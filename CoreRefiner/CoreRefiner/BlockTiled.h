#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "CubeTiled.h"
#include "Channels.h"
#include "Player.h"

class BlockTiled : public Environment
{
public:
	BlockTiled(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 size, XMFLOAT2 numTiled, bool onCollision, Object_Type_Tag tag = environment_BlockTiled)
		:
		Environment(tag)
	{
		pPlayer = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);

		// parameters init
		SetPosition(position);
		SetSize(size);
		SetCollisionSize(size);
		SetCollisionOnOff(onCollision);

		// graphics init
		visualPre = std::make_unique<CubeTiled>(gfx, size, numTiled);
		visualPre->LinkTechniques(rg);
		visualPre->SetPosition(transInfo.position);
		visualPre->GameStartExpend();

		// collider init
		DirectX::XMFLOAT3 localHalf{ 0.5f, 0.5f, 0.5f };
		boxCollider = BoxCollider::BuildFromWorldMatrix(transInfo.GetWorldMatrix(), localHalf);
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(1.0f, 0.0f, 0.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override 
	{
		visualPre->GameStartExpend();
	}
	void Update(float dt) override
	{
		auto playerPos = pPlayer->GetPosition();

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
					visualPre->StartExpand(playerPos, fieldMode);
			}
		}

		if (!pPlayer->GetIsSkill() && fieldPlay)
		{
			fieldPlay = false;
			fieldMode = 0;
		}

		visualPre->Update(dt, playerPos);
	}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
		visualPre->Submit(Chan::shadow);
#ifdef _DEBUG
		//boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
#endif
	}
	void OnCollide(Character* other) override {}
private:
	std::unique_ptr<CubeTiled> visualPre;
	Player* pPlayer;
	int fieldMode{ 0 };
	bool fieldPlay{ false };
};