#pragma once
#include "Graphics.h"
#include "Sprite2D.h"
#include "Channels.h"
#include "Player_ResourceSystem.h"

class UI_WeaponSlot
{
public:
	UI_WeaponSlot(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		// WeaponSlot_Bg
		{
			Bg = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Weapon_Slot.png" });
			Bg->SetPosition(SCREEN_WIDTH - 260.0f, SCREEN_HEIGHT - 50.0f);
			Bg->SetScale(520.0f, 520.0f);
			Bg->SetFrameAuto(8, 8, 1, 64, 1, 0, false, true);
			Bg->LinkTechniques(rg);
		}
		// WeaponSlot_Chosen
		{
			Chosen = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Weapon_Slot_Chosen.png" });
			Chosen->SetFrameAuto(29, 7, 30, 29 * 4, 1, 0, false, true);
			Chosen->LinkTechniques(rg);
		}
		// WeaponSlot_Icon
		float baseX = SCREEN_WIDTH - 460.0f;
		float baseY = SCREEN_HEIGHT - 80.0f;
		float gap = 79.0f;
		for (int i = 0; i < totalIcon; i++)
		{
			// WeaponSlot_Icon
			Icon.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Weapon_Slot_Icon.png" }));
			Icon[i]->SetPosition(baseX + gap * i, baseY);
			Icon[i]->SetScale(iconSize, iconSize);
			Icon[i]->SetFrameAuto(29, 21, 1, 29 * 21, 1, 20.0f, false, true);
			Icon[i]->LinkTechniques(rg);
			// WeaponSlot_Icon_Bg
			float scale = 1.2f;
			IconBg.push_back(std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ "asset\\Images\\UI_Game\\Weapon_Slot_Icon_Bg.png" }));
			IconBg[i]->SetPosition(baseX + gap * i, baseY);
			IconBg[i]->SetScale(iconSize * scale, iconSize * scale);
			IconBg[i]->SetFrameAuto(29, 21, 1, 29 * 21, 1, 20.0f, false, true);
			IconBg[i]->LinkTechniques(rg);
		}
	}
	~UI_WeaponSlot() = default;

	void Update(float dt, WeaponSlots* ws)
	{
		auto w = ws->GetAllSlots();
		// WeaponSlot_Bg
		Bg->Update(dt);
		// WeaponSlot_Icon
		int u = 29, v = 7;
		int block = u * v;
		for (size_t i = 0; i < totalIcon; i++)
		{
			Icon[i]->Update(dt);
			IconBg[i]->Update(dt);
			switch (infos[i].state)
			{
			case NONE:
				if (infos[i].type != w[i + 1].GetType())
				{
					infos[i].state = APPEAR;
					infos[i].type = w[i + 1].GetType();
					int start = (4 - static_cast<int>(infos[i].type)) * block + 1;
					Icon[i]->SetFrameAuto(29, 21, start, 29, 0, 20.0f, false, false);
					Icon[i]->SetScale(iconSize, iconSize);
					IconBg[i]->SetFrameAuto(29, 21, start, 29, 0, 20.0f, false, false);
				}
				break;
			case APPEAR:
				if (Icon[i]->ClipFinished())
				{
					infos[i].state = IDLE;
					int start = (4 - static_cast<int>(infos[i].type)) * block + 1 + 29;
					Icon[i]->SetFrameAuto(29, 21, start, u * 4, 0, 20.0f, false, true);
					IconBg[i]->SetFrameAuto(29, 21, start, u * 4, 0, 20.0f, false, true);
				}
				break;
			case IDLE:
			{
				float x = std::clamp(w[i + 1].GetDrawParameterRate(), 0.0f, 1.0f);
				float a = 0.1f, b = 1.0f;
				float rate = a + x * (b - a);
				float size = iconSize * rate;
				Icon[i]->SetScale(size, size);
				if(infos[i].type != w[i + 1].GetType())
				{
					if (w[i + 1].GetType() != WEAPON_TYPE_NONE) 
					{
						infos[i].type = w[i + 1].GetType();
						int start = (4 - static_cast<int>(infos[i].type)) * block + 1 + 29;
						Icon[i]->SetFrameAuto(29, 21, start, u * 4, 0, 20.0f, false, true);
						IconBg[i]->SetFrameAuto(29, 21, start, u * 4, 0, 20.0f, false, true);
					}
					else
					{
						infos[i].state = DISAPPEAR;
						int start = (4 - static_cast<int>(infos[i].type)) * block + 1 + 29 * 5;
						if (start >= 3)
						{
							infos[i].state = NONE;
							infos[i].type = WEAPON_TYPE_NONE;
						}
						Icon[i]->SetFrameAuto(29, 21, start, 29 + 8, 0, 20.0f, false, false);
						IconBg[i]->SetFrameAuto(29, 21, start, 29 + 8, 0, 20.0f, false, false);
					}
				}
			}
				break;
			case DISAPPEAR:
				if (Icon[i]->ClipFinished())
				{
					infos[i].state = NONE;
					infos[i].type = WEAPON_TYPE_NONE;
				}
				break;
			}
		}
		// WeaponSlot_Chosen
		{
			chosenIndex = ws->GetCurrentSlotNum() - 1;
			Chosen->Update(dt);
			if (chosenIndex >= 0 && chosenIndex < totalIcon)
			{
				float outlineScale = 1.25f;
				auto pos = Icon[chosenIndex]->GetPosition();
				auto size = Icon[chosenIndex]->GetScale();
				Chosen->SetPosition(pos.x, pos.y);
				Chosen->SetScale(size.x * outlineScale, size.y * outlineScale);
			}
		}
	}
	void Submit(void)
	{
		// WeaponSlot_Bg
		Bg->Submit(Chan::ui);		
		// WeaponSlot_Icon
		for (size_t i = 0; i < totalIcon; i++)
		{
			if (infos[i].state != NONE)
			{				
				// WeaponSlot_Icon_Bg
				IconBg[i]->Submit(Chan::ui);
				// WeaponSlot_Chosen
				if (i == chosenIndex) Chosen->Submit(Chan::ui);
				// WeaponSlot_Icon
				Icon[i]->Submit(Chan::ui);
			}
		}
	}
private:
	// WeaponSlot_Bg
	std::unique_ptr<Sprite2D> Bg;
	// WeaponSlot_Chosen
	int chosenIndex = -1;
	std::unique_ptr<Sprite2D> Chosen;
	// WeaponSlot_Icon
	static constexpr int totalIcon = 6;
	static constexpr float iconSize = 110.0f;
	std::vector<std::unique_ptr<Sprite2D>> Icon;
	std::vector<std::unique_ptr<Sprite2D>> IconBg;
	enum SlotState
	{
		NONE,
		APPEAR,
		IDLE,
		DISAPPEAR,
	};
	struct SlotInfo
	{
		SlotState state = NONE;
		WEAPON_TYPE_ID type = WEAPON_TYPE_NONE;
	} infos[totalIcon];
};