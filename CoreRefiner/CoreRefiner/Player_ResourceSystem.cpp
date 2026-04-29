#include "Player_ResourceSystem.h"
#include "GameStatsCodex.h"
#include "SoundCodex.h"

/*------------------------------------------------------------------------------
   Weapon
------------------------------------------------------------------------------*/
void Weapon::Update(float dt)
{
	if (EnergyDraw != Energy)
		EnergyDraw += (Energy - EnergyDraw) * 0.1f;

	if (EnergyDraw - Energy <= 0.1f && Energy <= 0.0f)
		SetWaitToDiscard(true);
}


/*------------------------------------------------------------------------------
   WeaponSlots
------------------------------------------------------------------------------*/
void WeaponSlots::Update(float dt)
{
	if (!IsFever)
	{
		int SameCount = 0;
		WEAPON_TYPE_ID SameType = WEAPON_TYPE_NONE;

		for (int i = 1;i < Slots.size();)
		{
			// もし空きスロットがあったら、後ろは全部空きスロット
			if (Slots[i].GetType() == WEAPON_TYPE_NONE)
				break;

			Slots[i].Update(dt);

			if (Slots[i].GetWaitToDiscard())
			{
				DiscardWeapon(i);
				Slots[i].SetWaitToDiscard(false);
			}
			else
			{
				if (SameCount < SlotNeeded)
				{
					if (SameType == Slots[i].GetType())
						SameCount++;
					else
					{
						SameType = Slots[i].GetType();
						SameCount = 1;
					}
				}
				i++;
			}
		}

		CanChange = { SameCount >= SlotNeeded ,SameType };
	}
	else if (IsFever && IsChange)
	{
		for (int i = 1;i < Slots.size();i++)
		{
			// If there is an empty slot, all the remaining slots are empty.
			if (Slots[i].GetType() == WEAPON_TYPE_NONE)
				break;

			Slots[i].Update(dt);

			if (Slots[i].GetWaitToDiscard())
			{
				DiscardWeapon(i);
				Slots[i].SetWaitToDiscard(false);
			}
		}
	}
	else
	{
		FeverTime -= dt;

		if (FeverTimeDraw != FeverTime)
			FeverTimeDraw += (FeverTime - FeverTimeDraw) * 0.1f;

		if (FeverTime - FeverTimeDraw <= 0.1f && FeverTime <= 0.0f)
			IsFever = false;
	}
}

void WeaponSlots::SwitchSlot(bool RightOrLeft)
{
	if (RightOrLeft)
	{
		if (--CurrentSlot < 0)
		{
			CurrentSlot = TotalSlots - 1;
			while (Slots[CurrentSlot].GetType() == WEAPON_TYPE_NONE)
				CurrentSlot--;
		}
	}
	else
	{
		CurrentSlot++;
		if (CurrentSlot >= TotalSlots || Slots[CurrentSlot].GetType() == WEAPON_TYPE_NONE)
			CurrentSlot = 0;
	}
}

void WeaponSlots::SetNewWeapon(WEAPON_TYPE_ID type)
{
	if (Slots[TotalSlots - 1].GetType() != WEAPON_TYPE_NONE)
		return;

	int slot = TotalSlots - 2;
	while (Slots[slot].GetType() == WEAPON_TYPE_NONE)
		slot--;

	Slots[slot + 1].SetWeapon(type, 100.0f);

	GameStatsCodex::AddTotalWeapon();
	switch (type)
	{
	case WEAPON_TYPE_1:	
		SoundCodex::Get().PlaySE(SndPath::SE_Player_Weapon_Get_Red);
		GameStatsCodex::AddRedWeapon();
		break;
	case WEAPON_TYPE_2:	
		SoundCodex::Get().PlaySE(SndPath::SE_Player_Weapon_Get_Green);
		GameStatsCodex::AddGreenWeapon();
		break;
	case WEAPON_TYPE_3:	
		SoundCodex::Get().PlaySE(SndPath::SE_Player_Weapon_Get_Blue);
		GameStatsCodex::AddBlueWeapon();
		break;
	}
}

void WeaponSlots::UseWeapon(float offset)
{
	Slots[CurrentSlot].CalculateEnergy(offset);
}

void WeaponSlots::DoChange(void)
{
	int MaxCount = SlotNeeded;
	int num = 1;

	while (MaxCount > 0)
	{
		if (CanChange.second == Slots[num].GetType())
		{
			Slots[num].CalculateEnergy(-100);
			MaxCount--;
		}
		num++;
	}

	IsChange = true;
	IsFever = true;
	CanChange = { false,WEAPON_TYPE_NONE };
}

void WeaponSlots::FinishChange(void)
{
	IsChange = false;
	FeverTime = FeverCountdown;
}

void WeaponSlots::DiscardWeapon(int num)
{
	int slot = num;
	while (++slot < TotalSlots)
	{
		if (Slots[slot].GetType() != WEAPON_TYPE_NONE)
			Slots[slot - 1].SetWeapon(Slots[slot].GetType(), Slots[slot].GetEnergy());
		else
			break;
	}
	Slots[slot - 1].SetType(WEAPON_TYPE_NONE);

	if (CurrentSlot == slot - 1)
		CurrentSlot--;
}


/*------------------------------------------------------------------------------
   ResourceBars
------------------------------------------------------------------------------*/
void ResourceBars::Update(float dt)
{
	if (!Slots->GetIsFever())
	{
		// calculating the value to draw on the energy bar
		for (int i = 0;i < TotalWeaponTypes;i++)
			if (EnergyDraw[i] != EnergyCumulated[i])
				EnergyDraw[i] += (EnergyCumulated[i] - EnergyDraw[i]) * 0.2f;

		// check if get a new weapon
		for (int i = 0;i < TotalWeaponTypes;i++)
			if (EnergyCumulated[i] - EnergyDraw[i] < 1.0f && EnergyCumulated[i] >= EnergyLimit)
			{
				Slots->SetNewWeapon(static_cast<WEAPON_TYPE_ID>(TotalWeaponTypes + i - 1));
				EnergyCumulated[i] = 0.0f;
				EnergyFull[i] = true;
			}
	}

	// countdown and reset of combo time
	for (int i = 0;i < TotalWeaponTypes;i++)
	{
		if (AttackComboCountdowns[i] > 0.0f)	AttackComboCountdowns[i] -= dt;
		if (AttackComboCountdowns[i] <= 0.0f)	AttackCombos[i] = 0;
	}
}

void ResourceBars::AttackSettlement(int enemyType, bool isFever)
{
	AttackCombos[enemyType - 1]++;
	AttackComboCountdowns[enemyType - 1] = ComboCountdown;

	switch (enemyType)
	{
	case 1:	GameStatsCodex::ReportRedCombo(AttackCombos[enemyType - 1]);	break;
	case 2:	GameStatsCodex::ReportGreenCombo(AttackCombos[enemyType - 1]);	break;
	case 3:	GameStatsCodex::ReportBlueCombo(AttackCombos[enemyType - 1]);	break;
	}

	if (!isFever)
	{
		// add the same amount of energy as the number of combos
		EnergyCumulated[enemyType - 1] += (AttackCombos[enemyType - 1] > 10 ? 10 : AttackCombos[enemyType - 1]) * 2;
		if (EnergyCumulated[enemyType - 1] > 100.0f) EnergyCumulated[enemyType - 1] = 100.0f;

		// when the number of combos is a multiple of 5, add energy
		//EnergyCumulated[enemyType - 1] += AttackCombos[enemyType - 1] % 5 == 0 ? AttackCombos[enemyType - 1] / 5 * 5 : 0;
		//EnergyCumulated[enemyType - 1] = std::min(EnergyCumulated[enemyType - 1], 100.0f);
	}
}