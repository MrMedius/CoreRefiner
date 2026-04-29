#pragma once
#include "Character.h"
#include "RenderGraph.h"
#include "ObjectCodex.h"
#include "CameraContainer.h"
#include "Sprite3D.h"
#include "Sprite3DNoLit.h"
#include "Sprite2D.h"
#include "Player_ResourceSystem.h"

// プレイヤー状態のID
enum PLAYER_STATE_ID {
	PLAYER_IDLE,
	PLAYER_MOVE,
	PLAYER_DASH,
	PLAYER_ATTACK,
	PLAYER_SKILL,
	PLAYER_HURT,
	PLAYER_DEATH,
	PLAYER_CHANGE,
};
// プレイヤー状態の名前
static const std::string PLAYER_STATE[] = {
	"PLAYER_IDLE",
	"PLAYER_MOVE",
	"PLAYER_DASH",
	"PLAYER_ATTACK",
	"PLAYER_SKILL",
	"PLAYER_HURT",
	"PLAYER_DEATH",
	"PLAYER_CHANGE",
};
class Player_IdleState;
class Player_MoveState;
class Player_DashState;
class Player_AttackState;
class Player_SkillState;
class Player_HurtState;
class Player_DeathState;
class Player_ChangeState;

class Player : public Character
{
public:
	Player(Graphics& gfx, Rgph::RenderGraph& rg, CameraContainer* camera, XMFLOAT3 position, Object_Type_Tag tag = character_Player)
		:
		Character(tag),
		Gfx(gfx),
		Rg(rg),
		pCamera(camera)
	{
		// control init
		BindPad(0);

		// parameters init
		SetPosition(position);
		SetSize({ 5.0f, 5.0f, 0.0f });
		SetCollisionSize({ 1.5f, 4.0f, 2.0f });
		SetCollisionOnOff(true);
		SetHpMax(30.0f);
		ResetHpCurrent();
		SetMoveAccel(0.035f);
		SetAttackCollisionSize({ 4.0f,6.0f,2.5f });
		SetAttackInterval(1.5f);

		// game resources init
		pWeapon = std::make_unique<WeaponSlots>();
		pResource = std::make_unique<ResourceBars>(pWeapon.get());

		// graphics init
		// visualPre
		{
			visualPre = std::make_unique<Sprite3D>(gfx, std::vector<std::string>{
					"asset\\Images\\Player\\Player_Usual.png",
					"asset\\Images\\Player\\Player_Red.png",
					"asset\\Images\\Player\\Player_Green.png",
					"asset\\Images\\Player\\Player_Blue.png",
					"asset\\Images\\Player\\Player_Fever.png"});
			visualPre->SetPosition(transInfo.position);
			visualPre->SetScale(transInfo.scale);
			visualPre->LinkTechniques(rg);
		}
		// visualPre_Effect_Bg
		{
			visualPre_Effect_Bg = std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{
					"asset\\Images\\Player\\Player_Usual_Effect_Bg.png",
					"asset\\Images\\Player\\Player_Red_Effect_Bg.png",
					"asset\\Images\\Player\\Player_Green_Effect_Bg.png",
					"asset\\Images\\Player\\Player_Blue_Effect_Bg.png",
					"asset\\Images\\Player\\Player_Red_Effect.png",
					"asset\\Images\\Player\\Player_Green_Effect.png",
					"asset\\Images\\Player\\Player_Blue_Effect.png",
					"asset\\Images\\Player\\Player_Fever_Bg_Back.png",
					"asset\\Images\\Player\\Player_Transform_Fever.png"});
			visualPre_Effect_Bg->SetPosition(transInfo.position);
			visualPre_Effect_Bg->SetScale(GetEffectSize());
			visualPre_Effect_Bg->SetFrameAuto(12, 10, 1, 120, 0, 20.0f, false, true);
			visualPre_Effect_Bg->LinkTechniques(rg);
		}
		// visualPre_Effect
		{
			visualPre_Effect = std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{
					"asset\\Images\\Player\\Player_Usual_Effect.png",
					"asset\\Images\\Player\\Player_Red_Effect.png",
					"asset\\Images\\Player\\Player_Green_Effect.png",
					"asset\\Images\\Player\\Player_Blue_Effect.png",
					"asset\\Images\\Player\\Player_Fever_Bg_Front.png",
					"asset\\Images\\Player\\Player_Transform_Fever.png"});
			visualPre_Effect->SetScale(GetEffectSize());
			visualPre_Effect->LinkTechniques(rg);
		}
		// visualPre_Switch
		for (int i = 0;i < 2;i++)
		{
			visualPre_Switch.push_back(std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{"asset\\Images\\Player\\Player_Transform.png"}));
			visualPre_Switch[i]->SetScale(GetEffectSize());
			visualPre_Switch[i]->LinkTechniques(rg);
		}
		// visualPre_Fever
		for (int i = 0;i < 6;i++)
		{
			auto scale = 5.0f;
			visualPre_Fever.push_back(std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{"asset\\Images\\Player\\Player_Fever_Attack.png"}));
			visualPre_Fever[i]->SetScale(transInfo.scale.x * scale, transInfo.scale.y * scale);
			visualPre_Fever[i]->SetFrame(9, 8, 49, 24, 1, 0, (i < 3) ? true : false);
			visualPre_Fever[i]->LinkTechniques(rg);
		}
		// visualPre_Fever_Mega
		{
			
			visualPre_Fever_Mega = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{"asset\\Images\\Player\\Player_Fever_Blink.png", "asset\\Images\\Player\\Player_Fever_Mega.png" });
			visualPre_Fever_Mega->SetPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
			visualPre_Fever_Mega->SetScale(SCREEN_WIDTH * 1.1f, SCREEN_HEIGHT * 1.1f);
			visualPre_Fever_Mega->LinkTechniques(rg);
		}

		// FSM state init
		FSM = std::make_unique<StateMachine<Player>>(this);
		FSM->AddState(PLAYER_STATE[PLAYER_IDLE],	std::make_unique<Player_IdleState>());
		FSM->AddState(PLAYER_STATE[PLAYER_MOVE],	std::make_unique<Player_MoveState>());
		FSM->AddState(PLAYER_STATE[PLAYER_DASH],	std::make_unique<Player_DashState>());
		FSM->AddState(PLAYER_STATE[PLAYER_ATTACK],	std::make_unique<Player_AttackState>());
		FSM->AddState(PLAYER_STATE[PLAYER_SKILL],	std::make_unique<Player_SkillState>());
		FSM->AddState(PLAYER_STATE[PLAYER_HURT],	std::make_unique<Player_HurtState>());
		FSM->AddState(PLAYER_STATE[PLAYER_DEATH],	std::make_unique<Player_DeathState>());
		FSM->AddState(PLAYER_STATE[PLAYER_CHANGE],	std::make_unique<Player_ChangeState>());

		// FSM state transitions init
		SetupTransitions();
		// init the state
		FSM->ChangeState(PLAYER_STATE[PLAYER_IDLE]);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 1.0f, 0.0f, 0.0f }, "wireBox");
		boxColliderWire->LinkTechniques(rg);
		attackColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, "wireAttack");
		attackColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override
	{
		FSM->ChangeState(PLAYER_STATE[PLAYER_IDLE]);
		SetPosition(XMFLOAT3(0.0f, 45.0f, 0.0f));
		MoveVelocity = { 0.0f,0.0f,0.0f };
		ResetHpCurrent();
		SetIsDeath(false);
		SetIsGameOver(false);
		pResource->Reset();
		pWeapon->Reset();
		IsDash = false;
		IsSkill = false;
		WasSkill = false;
		IsChange = false;
		IsFever = false;
		isSwitch = false;
		IsFeverMega = false;
		IsGameOver = false;
		SetBackground();
		SetAttackCollisionSize({ 4.0f,6.0f,2.5f });
		SetAttackCollisionOnOff(false);
		SetDoAttackType(Attack_Type_None);
		SetFeverType(WEAPON_TYPE_0);
	}
	void Update(float dt) override;
	void Submit(void) override;
	float GetHpDrawParameter(void)
	{
		if (HpDraw != HpCurrent)
			HpDraw += (HpCurrent - HpDraw) * 0.2f;
		return HpDraw / HpMax;
	}
	ResourceBars* GetResourceBars(void) const	{ return pResource.get(); }
	WeaponSlots* GetWeaponSlots(void) const		{ return pWeapon.get(); }
	void SetIsDash(bool state)	 { IsDash = state; }
	bool GetIsDash(void) const	 { return IsDash; }
	void SetIsSkill(bool state)	 { IsSkill = state; }
	bool GetIsSkill(void) const	 { return IsSkill; }
	void SetWasSkill(bool state) { WasSkill = state; }
	bool GetWasSkill(void)
	{
		if (WasSkill)
		{
			WasSkill = false;
			return true;
		}
		else
			return false;
	}
	XMFLOAT2 GetEffectSize(void) const { float effectScale_rate = 1.5f; return {transInfo.scale.x * effectScale_rate, transInfo.scale.y * effectScale_rate}; }
	void SetIsChange(bool state) { IsChange = state; }
	bool GetIsChange(void) const { return IsChange; }
	void SetIsFever(bool state)  { IsFever = state; }
	bool GetIsFever(void) const  { return IsFever; }
	void SetIsFeverMega(bool state) { IsFeverMega = state; }
	bool GetIsFeverMega(void) const { return IsFeverMega; }
	void SetFeverType(WEAPON_TYPE_ID type)	{ FeverType = type; }
	WEAPON_TYPE_ID GetFeverType(void) const { return FeverType; }
	void SetIsGameOver(bool state) { IsGameOver = state; }
	bool GetIsGameOver(void) const { return IsGameOver; }
	void DoMove(float ratio);
	bool AttackCollide(float damage, XMFLOAT3 repel) override;
	void AttackCameraShake(int frames, float minRange, float maxRange);
	Sprite3D* GetVisualPre(void) { return visualPre.get(); }
	Sprite3DNoLit* GetVisualPreEffectBg(void) { return visualPre_Effect_Bg.get(); }
	Sprite3DNoLit* GetVisualPreEffect(void) { return visualPre_Effect.get(); }
	std::vector<Sprite3DNoLit*> GetVisualPreFever(void) 
	{ 
		std::vector<Sprite3DNoLit*> vpf;
		for (int i = 0;i < visualPre_Fever.size();i++)
			vpf.push_back(visualPre_Fever[i].get());
		return vpf;
	}
	Sprite2D* GetVisualPreFeverMega(void) { return visualPre_Fever_Mega.get(); }
	void SetBackground(void);
private:
	void SetupTransitions(void) override;
private:
	std::unique_ptr<Sprite3D> visualPre;
	std::unique_ptr<Sprite3DNoLit> visualPre_Effect_Bg;
	std::unique_ptr<Sprite3DNoLit> visualPre_Effect;
	std::vector<std::unique_ptr<Sprite3DNoLit>> visualPre_Switch;
	std::vector<std::unique_ptr<Sprite3DNoLit>> visualPre_Fever;
	std::unique_ptr<Sprite2D> visualPre_Fever_Mega;
	Graphics& Gfx;
	Rgph::RenderGraph& Rg;
	CameraContainer* pCamera;
	std::unique_ptr<StateMachine<Player>> FSM;
	std::unique_ptr<ResourceBars> pResource;
	std::unique_ptr<WeaponSlots> pWeapon;
	WEAPON_TYPE_ID currentWeaponType{ WEAPON_TYPE_NONE };
	bool isSwitch{ false };
	float HpDraw{ 0.0f };
	bool IsDash{ false };
	bool IsSkill{ false };
	bool WasSkill{ false };
	bool IsChange{ false };
	bool IsFever{ false };
	bool IsFeverMega{ false };
	WEAPON_TYPE_ID FeverType{ WEAPON_TYPE_0 };
	bool IsGameOver{ false };

// input related
	struct PlayerInputSnapshot
	{
		int padIndex = 0;

		float moveX = 0.0f;
		float moveZ = 0.0f;

		bool moveHeld = false;

		bool dash = false;
		bool attack = false;
		bool skill = false;
		bool change = false;
		bool slotL = false;
		bool slotR = false;

		void ClearOneShots() noexcept
		{
			dash = attack = skill = change = false;
		}
	};
public:
	const PlayerInputSnapshot& Input() const noexcept { return inputSnap; }

	void BindPad(int idx) noexcept { boundPadIndex = idx; }   // 0..3
	void UseKeyboardMouse() noexcept { boundPadIndex = -1; }
	int GetBoundPad() const noexcept { return boundPadIndex; }
	PlayerInputSnapshot inputSnap{};
	int boundPadIndex = -1;
private:
	void BuildInputSnapshot();
};

/*------------------------------------------------------------------------------
   Player_IdleState
------------------------------------------------------------------------------*/
class Player_IdleState : public State<Player>
{
public:
	Player_IdleState();
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_IDLE]; }
private:
	std::vector<SpriteAnimeInfo> packs;
	SpriteAnimeManualAssistant assist;
	WEAPON_TYPE_ID wType{ WEAPON_TYPE_NONE };
};

/*------------------------------------------------------------------------------
   Player_MoveState
------------------------------------------------------------------------------*/
class Player_MoveState : public State<Player>
{
public:
	Player_MoveState();
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_MOVE]; }
private:
	std::vector<SpriteAnimeInfo> packs;
	SpriteAnimeManualAssistant assist;
};

/*------------------------------------------------------------------------------
   Player_DashState
------------------------------------------------------------------------------*/
class Player_DashState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_DASH]; }
private:
	SpriteAnimeInfo pack{ 12, 16, 121, 12 };
	bool isInput{ false };
};

/*------------------------------------------------------------------------------
   Player_AttackState
------------------------------------------------------------------------------*/
class Player_AttackState : public State<Player>
{
public:
	Player_AttackState();
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override;
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_ATTACK]; }
private:
	std::vector<SpriteAnimeInfo> packs;
	std::vector<SpriteAnimeInfo> ePacks;
	std::vector<SpriteAnimeInfo> fPacks;
	SpriteAnimeManualAssistant assist;
	XMFLOAT3 baseAttackCollisionSize{ 0.0f,0.0f, 0.0f };
	unsigned int mode{ 0 };
	bool isCombo{ false };
	int oldFrame{ 0 };
};

/*------------------------------------------------------------------------------
   Player_SkillState
------------------------------------------------------------------------------*/
class Player_SkillState : public State<Player>
{
public:
	Player_SkillState();
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override;
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_SKILL]; }
private:
	SpriteAnimeInfo pack{ 12, 12, 97, 48 };
	std::vector<SpriteAnimeInfo> ePacks;
	std::vector<SpriteAnimeInfo> bgPacks;
	XMFLOAT3 baseAttackCollisionSize{ 0.0f,0.0f, 0.0f };
	WEAPON_TYPE_ID WeaponType{ WEAPON_TYPE_NONE };
	int oldFrame{ 0 };
};

/*------------------------------------------------------------------------------
   Player_HurtState
------------------------------------------------------------------------------*/
class Player_HurtState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override;
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_HURT]; }
private:
	SpriteAnimeInfo pack{ 12, 16, 97, 6 };
};

/*------------------------------------------------------------------------------
   Player_DeathState
------------------------------------------------------------------------------*/
class Player_DeathState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override {};
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_DEATH]; }
private:
	SpriteAnimeInfo pack{ 12, 16, 1, 48 };
};

/*------------------------------------------------------------------------------
   Player_ChangeState
------------------------------------------------------------------------------*/
class Player_ChangeState : public State<Player>
{
public:
	void OnEnter(Player* owner) override;
	void OnExit(Player* owner) override;
	void Update(Player* owner, float dt) override;
	std::string GetName() const override { return PLAYER_STATE[PLAYER_CHANGE]; }
private:
	SpriteAnimeInfo packOut{ 12, 10, 49, 48 };
	SpriteAnimeInfo packIn{ 7,  7,  0, 24, 10.0f };
	int oldFrame{ 0 };
};