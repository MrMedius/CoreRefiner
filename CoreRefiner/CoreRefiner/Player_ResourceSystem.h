#pragma once
#include <vector>

class ResourceBars;
class WeaponSlots;
class Weapon;

// ïêäÌÇÃéÌóﬁ
enum WEAPON_TYPE_ID {
	WEAPON_TYPE_NONE,
	WEAPON_TYPE_0,
	WEAPON_TYPE_1,
	WEAPON_TYPE_2,
	WEAPON_TYPE_3,
};

/*------------------------------------------------------------------------------
   Weapon
------------------------------------------------------------------------------*/
class Weapon
{
public:
	Weapon() {};
	~Weapon() = default;
	void Update(float dt);
	void SetWeapon(WEAPON_TYPE_ID type, float energy)
	{
		SetType(type);
		SetEnergy(energy);
	}
	void SetType(WEAPON_TYPE_ID type) { Type = type; }
	WEAPON_TYPE_ID GetType(void) const { return Type; }
	void SetEnergy(float energy) { Energy = energy; }
	float GetEnergy(void) const { return Energy; }
	void CalculateEnergy(float offset) { if (Type != WEAPON_TYPE_0) Energy = std::max(Energy + offset, 0.0f); }
	float GetDrawParameterRate(void) const { return EnergyDraw / EnergyLimit; }
	void SetWaitToDiscard(bool discard) { WaitToDiscard = discard; }
	bool GetWaitToDiscard(void) const { return WaitToDiscard; }
private:
	static constexpr float EnergyLimit = 100.0f;
	WEAPON_TYPE_ID Type{ WEAPON_TYPE_NONE };
	float Energy{ 0.0f };
	float EnergyDraw{ 0.0f };
	bool WaitToDiscard{ false };
};


/*------------------------------------------------------------------------------
   WeaponSlots
------------------------------------------------------------------------------*/
class WeaponSlots
{
public:
	WeaponSlots()
	{
		for (int i = 0;i < TotalSlots;i++)
			Slots.push_back(Weapon());
		Slots[0].SetWeapon(WEAPON_TYPE_0, 100.0f);
	}
	~WeaponSlots() = default;
	void Update(float dt);
	void SwitchSlot(bool RightOrLeft);
	void UseWeapon(float offset);
	void SetNewWeapon(WEAPON_TYPE_ID type);
	void DoChange(void);
	void FinishChange(void);
	int GetCurrentSlotNum(void) const { return CurrentSlot; }
	int GetTotalSlotNum(void) const { return TotalSlots; }
	Weapon GetSlot(int num) const { return Slots[num]; }
	Weapon GetCurrentSlot(void) const { return Slots[CurrentSlot]; }
	std::pair<bool, WEAPON_TYPE_ID> GetCanChange(void) const { return CanChange; }
	std::vector<Weapon> GetAllSlots(void) const { return Slots; }
	bool GetIsFever(void) const { return IsFever; }
	float GetFeverTimeDraw(void) const { return FeverTimeDraw / FeverCountdown; }
	void Reset(void)
	{
		// normal slots
		for (int i = 1;i < TotalSlots;i++) Slots[i].SetWeapon(WEAPON_TYPE_NONE, 0.0f);
		CurrentSlot = 0;
		// Fever related
		CanChange = { false,WEAPON_TYPE_NONE };
		IsChange = false;
		IsFever = false;
		FeverTime = 0.0f;
		FeverTimeDraw = 0.0f;
	}
private:
	void DiscardWeapon(int num);
private:
	// normal slots
	static constexpr int TotalSlots = 7;
	int CurrentSlot{ 0 };
	std::vector<Weapon> Slots;
	// Fever related
	static constexpr int SlotNeeded = 3;
	std::pair<bool, WEAPON_TYPE_ID> CanChange{ false,WEAPON_TYPE_NONE };
	bool IsChange{ false };
	static constexpr float FeverCountdown = 15.0f;
	bool IsFever{ false };
	float FeverTime{ 0.0f };
	float FeverTimeDraw{ 0.0f };
};


/*------------------------------------------------------------------------------
   ResourceBars
------------------------------------------------------------------------------*/
class ResourceBars
{
public:
	ResourceBars(WeaponSlots* slots) : Slots(slots)
	{
		for (int i = 0;i < TotalWeaponTypes;i++)
		{
			EnergyCumulated.push_back(0.0f);
			EnergyDraw.push_back(0.0f);
			EnergyFull.push_back(false);
			EnergyChanged.push_back(false);
			AttackCombos.push_back(0);
			AttackComboCountdowns.push_back(0.0f);
		}
	};
	~ResourceBars() = default;
	void Update(float dt);
	void AttackSettlement(int enemyType, bool isFever);
	int GetTotalWeaponTypes(void) const { return TotalWeaponTypes; }
	std::vector<float> GetEnergyDrawParameters(void) const
	{
		std::vector<float> EnergyDrawParameters;
		for (auto p : EnergyDraw) EnergyDrawParameters.push_back(p / EnergyLimit);
		return EnergyDrawParameters;
	}
	std::vector<bool> GetEnergyFull(void) const
	{
		return EnergyFull;
	}
	void SetEnergyFullAt(size_t i, bool v) 
	{ 
		EnergyFull[i] = v; 
	}
	bool GetEnergyChangedAt(size_t i)
	{
		if (EnergyChanged[i])
		{
			EnergyChanged[i] = false;
			return true;
		}
		return false;
	}
	void SetEnergyChangedAt(size_t i, bool v)
	{
		EnergyChanged[i] = v;
	}
	std::vector<unsigned int> GetAttackCombos(void) const
	{
		return AttackCombos;
	}
	std::vector<float> GetAttackComboCountdowns(void) const
	{
		return AttackComboCountdowns;
	}
	std::vector<float> GetAttackComboCountdownRates(void) const
	{
		std::vector<float> rates;
		for (auto cd : AttackComboCountdowns)
			rates.push_back(cd / ComboCountdown);
		return rates;
	}
	void ResetCombo(void)
	{
		std::fill(AttackCombos.begin(),			 AttackCombos.end(),		  0u);
		std::fill(AttackComboCountdowns.begin(), AttackComboCountdowns.end(), 0.0f);
	}
	void Reset(void)
	{
		std::fill(EnergyCumulated.begin(),		 EnergyCumulated.end(),		  0.0f);
		std::fill(EnergyDraw.begin(),			 EnergyDraw.end(),			  0.0f);
		std::fill(EnergyFull.begin(),			 EnergyFull.end(),			  false);
		std::fill(EnergyChanged.begin(),		 EnergyChanged.end(),		  false);
		std::fill(AttackCombos.begin(),			 AttackCombos.end(),		  0u);
		std::fill(AttackComboCountdowns.begin(), AttackComboCountdowns.end(), 0.0f);
	}
private:
	WeaponSlots* Slots;
	static constexpr int TotalWeaponTypes = 3;
	static constexpr float EnergyLimit = 100.0f;
	static constexpr float ComboCountdown = 2.0f;
	std::vector<float> EnergyCumulated;
	std::vector<float> EnergyDraw;
	std::vector<bool> EnergyFull;
	std::vector<bool> EnergyChanged;
	std::vector<unsigned int> AttackCombos;
	std::vector<float> AttackComboCountdowns;
};