#include "Player.h"
#include "Enemy.h"
#include "Channels.h"

#include "InputCodex.h"
#include "SoundCodex.h"
#include "GameStatsCodex.h"

static void Normalize2D(float& x, float& z) noexcept
{
	float len2 = x * x + z * z;
	if (len2 <= 0.000001f) return;
	float invLen = 1.0f / sqrtf(len2);
	x *= invLen;
	z *= invLen;
}

void Player::Update(float dt)
{
	BuildInputSnapshot();

	// 変更前の座標を格納
	auto Position = transInfo.position;
	PositionOld = Position;
	// 攻撃範囲の更新
	if (!IsFlip) attackCollider.center = { Position.x + boxCollider.half.x + attackCollider.half.x, Position.y, Position.z };
	else attackCollider.center = { Position.x - boxCollider.half.x - attackCollider.half.x, Position.y, Position.z };

	// 状態遷移
	// Fever related
	if (!IsChange && !IsDeath)
	{
		if ((inputSnap.change && GetWeaponSlots()->GetCanChange().first) || // Get into Fever
			(IsFever && !GetWeaponSlots()->GetIsFever())) // Exit Fever
		{
			SetIsChange(true);	// Feverに入る
		}
	}

	// FSM update
	FSM->Update(dt);
	visualPre_Effect_Bg->Update(dt);
	if(IsFever) visualPre_Effect->Update(dt);
	if (isSwitch)
	{
		visualPre_Switch[0]->Update(dt);
		visualPre_Switch[1]->Update(dt);
	}

	// Weapon ralated update
	{
		if (!FSM->IsInState("PLAYER_ATTACK") && !FSM->IsInState("PLAYER_SKILL") && !IsFever)
		{
			bool switched = false;
			if (inputSnap.slotL)
			{
				switched = true;
				pWeapon->SwitchSlot(true);
			}
			if (inputSnap.slotR) 
			{
				switched = true;
				pWeapon->SwitchSlot(false);
			}

			// visualPre_Switch
			if (switched)
			{
				auto type = pWeapon->GetCurrentSlot().GetType();
				if (type != WEAPON_TYPE_0)
				{
					isSwitch = true;

					switch (type)
					{
					case WEAPON_TYPE_1: visualPre_Switch[0]->SetFrameAuto(8, 6, 17, 8, 0, 20.0f, IsFlip, false); visualPre_Switch[1]->SetFrameAuto(8, 6, 41, 8, 0, 20.0f, IsFlip, false); break;
					case WEAPON_TYPE_2: visualPre_Switch[0]->SetFrameAuto(8, 6,  9, 8, 0, 20.0f, IsFlip, false); visualPre_Switch[1]->SetFrameAuto(8, 6, 33, 8, 0, 20.0f, IsFlip, false); break;
					case WEAPON_TYPE_3: visualPre_Switch[0]->SetFrameAuto(8, 6,  1, 8, 0, 20.0f, IsFlip, false); visualPre_Switch[1]->SetFrameAuto(8, 6, 25, 8, 0, 20.0f, IsFlip, false); break;
					}
				}
			}
		}
		if ((currentWeaponType != pWeapon->GetCurrentSlot().GetType() && !FSM->IsInState("PLAYER_SKILL")) || // Not in Skill
			GetWasSkill()) // after Skill
		{
			SetBackground();
		}
		pResource->Update(dt);
		pWeapon->Update(dt);
	}

	// 重力
	if (!OnFloor) MoveVelocity.y -= GRAVITY * dt;

	// 抵抗力
	MoveVelocity.x -= MoveVelocity.x * FORCE_RATE;
	MoveVelocity.z -= MoveVelocity.z * FORCE_RATE;

	// 移動
	{
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		visualPre->SetPosition(transInfo.position);
		if(!FSM->IsInState("PLAYER_SKILL") && !IsFever && !IsChange)
		{
			visualPre_Effect_Bg->SetPosition(transInfo.position.x, transInfo.position.y, transInfo.position.z + 0.1f);
			visualPre_Effect->SetPosition(attackCollider.center.x - (IsFlip ? -0.5f : 0.5f), attackCollider.center.y + 0.5f, attackCollider.center.z - 0.1f);
		}
		if (IsFever || IsChange)
		{
			visualPre_Effect_Bg->SetPosition(transInfo.position.x, transInfo.position.y, transInfo.position.z + 0.1f);
			visualPre_Effect->SetPosition(transInfo.position.x, transInfo.position.y, transInfo.position.z - 0.1f);

			switch (GetDoAttackType())
			{
			case Player_Fever_1: 
			case Player_Fever_2: 
				visualPre_Fever[0]->SetPosition(attackCollider.center.x - (IsFlip ? -3.0f : 3.0f), transInfo.position.y, transInfo.position.z + 0.1f);
				visualPre_Fever[1]->SetPosition(transInfo.position.x, transInfo.position.y + 3.0f, transInfo.position.z - 0.1f);
				break;
			case Player_Fever_3: 
				visualPre_Fever[0]->SetPosition(transInfo.position.x + 10.0f, transInfo.position.y + 7.5f, transInfo.position.z);
				visualPre_Fever[1]->SetPosition(transInfo.position.x +  5.0f, transInfo.position.y + 7.5f, transInfo.position.z + 10.0f);
				visualPre_Fever[2]->SetPosition(transInfo.position.x +  5.0f, transInfo.position.y + 7.5f, transInfo.position.z - 10.0f);
				visualPre_Fever[3]->SetPosition(transInfo.position.x - 10.0f, transInfo.position.y + 7.5f, transInfo.position.z);
				visualPre_Fever[4]->SetPosition(transInfo.position.x -  5.0f, transInfo.position.y + 7.5f, transInfo.position.z + 10.0f);
				visualPre_Fever[5]->SetPosition(transInfo.position.x -  5.0f, transInfo.position.y + 7.5f, transInfo.position.z - 10.0f);
				break;
			}
		}
		if (isSwitch)
		{
			visualPre_Switch[0]->SetPosition(transInfo.position.x, transInfo.position.y, transInfo.position.z - 0.1f);
			visualPre_Switch[1]->SetPosition(transInfo.position.x, transInfo.position.y, transInfo.position.z + 0.1f);

			if (visualPre_Switch[0]->ClipFinished()) isSwitch = false;
		}
	}

	// Fever Mega
	if (IsFeverMega)
		visualPre_Fever_Mega->Update(dt);
	if (IsFeverMega && visualPre_Fever_Mega->GetTexIndex() == 1 && visualPre_Fever_Mega->ClipFinished())
		IsFeverMega = false;

	// マップ要素との当たり判定
	boxCollider.center = transInfo.position;
	MapItemCollide();
}

void Player::Submit(void)
{
	// 描画
	// visualPre
	visualPre->Submit(Chan::main);
	visualPre->Submit(Chan::shadow);
	// visualPre_Effect_Bg
	visualPre_Effect_Bg->Submit(Chan::main);
	// visualPre_Effect
	if ((FSM->IsInState("PLAYER_ATTACK") && pWeapon->GetCurrentSlot().GetType() != WEAPON_TYPE_2) // attack state
		|| FSM->IsInState("PLAYER_SKILL")	// skill state
		|| FSM->IsInState("PLAYER_CHANGE")	// change state
		|| IsFever) // is in fever
		visualPre_Effect->Submit(Chan::main);
	// visualPre_Switch
	if (isSwitch)
	{
		visualPre_Switch[0]->Submit(Chan::main);
		visualPre_Switch[1]->Submit(Chan::main);
	}
	// visualPre_Fever
	if (IsFever)
	{
		switch (GetDoAttackType())
		{
		case Player_Fever_1:
		case Player_Fever_2:
			visualPre_Fever[0]->Submit(Chan::main);
			visualPre_Fever[1]->Submit(Chan::main);
			break;
		case Player_Fever_3:
			for (int i = 0;i < visualPre_Fever.size();i++)
				visualPre_Fever[i]->Submit(Chan::main);
			break;
		}
	}
	// visualPre_Fever_Mega
	if(IsFeverMega) visualPre_Fever_Mega->Submit(Chan::ui);


#ifdef _DEBUG
	if (!IsDeath)
	{
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
		attackColliderWire->DoSubmit(attackCollider.center, attackCollider.GetSize());
	}
#endif
}

void Player::SetBackground(void)
{
	currentWeaponType = pWeapon->GetCurrentSlot().GetType();
	switch (currentWeaponType)
	{
	case WEAPON_TYPE_0: visualPre_Effect_Bg->SetFrameAuto(12, 10, 1, 120, 0, 20.0f, false, true); break;
	case WEAPON_TYPE_1: visualPre_Effect_Bg->SetFrameAuto(10, 10, 1, 100, 1, 20.0f, false, true); break;
	case WEAPON_TYPE_2: visualPre_Effect_Bg->SetFrameAuto(12, 12, 1, 144, 2, 20.0f, false, true); break;
	case WEAPON_TYPE_3: visualPre_Effect_Bg->SetFrameAuto(12, 12, 1, 144, 3, 20.0f, false, true); break;
	}
}

void Player::DoMove(float ratio)
{
	if (inputSnap.moveHeld)
	{
		float dx = inputSnap.moveX;
		float dz = inputSnap.moveZ;

		float angle = atan2f(dx, dz);
		CalculateMoveVelocity(sinf(angle) * GetMoveAccel() * ratio, 0.0f, cosf(angle) * GetMoveAccel() * ratio);
	}
}

bool Player::AttackCollide(float damage, XMFLOAT3 repel)
{
	std::vector<Enemy*> enemies;
	for (auto tag : {
		character_Enemy_Red_T,
		character_Enemy_Red_0,
		character_Enemy_Red_1,
		character_Enemy_Green_T,
		character_Enemy_Green_0,
		character_Enemy_Green_1,
		character_Enemy_Blue_T,
		character_Enemy_Blue_0,
		character_Enemy_Blue_1
		}) {
		// プレイヤーを攻撃できる全ての敵を探す
		auto found = ObjectCodex::FindActiveObjectsByTag<Enemy>(tag);
		// 全ての敵を整理する
		enemies.reserve(enemies.size() + found.size());
		enemies.insert(enemies.end(), found.begin(), found.end());
	}

	for (auto e : enemies)
	{
		if (e->GetBeAttackedType() != GetDoAttackType() && !e->GetIsDeath())
		{
			bool isHit = CollisionSystem::IsOverlap(attackCollider, e->GetBoxCollider());

			if (isHit)
			{
				e->SetIsHurt(true);	// 攻撃された状態に遷移
				e->SetWasHurt(true);
				e->CalculateHpCurrent(damage); // 体力計算
				GameStatsCodex::AddOutputDamage(-damage);
				e->SetBeAttackedType(GetDoAttackType());
				SoundCodex::Get().PlaySE(SndPath::SE_Enemy_Hurt);

				float dx = e->GetPosition().x - transInfo.position.x;
				float dz = e->GetPosition().z - transInfo.position.z;
				float angle = atan2f(dx, dz);
				e->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z); // 撃退する
				//e->CalculateMoveVelocity(IsFlip ? -repel : repel, 0.0f, 0.0f); 

				// ゲームリソース判断(Feverじゃない時だけエネルギーがもらえる)
				pResource->AttackSettlement(e->GetEnemyType(), IsFever);

				// カメラ効果
				switch (DoAttackType)
				{
				case Player_Attack_1:	AttackCameraShake(4, 0.3f, 0.5f); InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.2f, 0.2f, 10); break;
				case Player_Attack_2:	AttackCameraShake(4, 0.5f, 1.0f); InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.3f, 0.3f, 20); break;
				case Player_Attack_3:	AttackCameraShake(6, 1.0f, 1.5f); InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.5f, 0.5f, 30); break;
				case Player_Skill_1:	AttackCameraShake(8, 2.0f, 3.0f); pCamera->SetScreenFroze(16); InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.8f, 0.8f, 80);  break;
				case Player_Skill_3:	AttackCameraShake(2, 0.5f, 1.0f); pCamera->SetScreenFroze(8);  InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.4f, 0.4f, 40);  break;
				case Player_Fever_1:	AttackCameraShake(4, 1.0f, 2.0f); pCamera->SetScreenFroze(4);  InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.5f, 0.5f, 16);  break;
				case Player_Fever_2:	AttackCameraShake(4, 1.0f, 2.0f); pCamera->SetScreenFroze(8);  InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.5f, 0.5f, 8);   break;
				case Player_Fever_3:	AttackCameraShake(6, 2.0f, 4.0f); pCamera->SetScreenFroze(12); InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 0.5f, 0.5f, 4);   break;
				case Player_Fever_Mega: AttackCameraShake(8, 3.0f, 5.0f); pCamera->SetScreenFroze(16); InputCodex::Get().GP_SetVibrationPulse(boundPadIndex, 1.0f, 1.0f, 100); break;
				}
			}
		}
	}
	return false;
}

void Player::AttackCameraShake(int frames, float minRange, float maxRange)
{
	pCamera->SetScreenShake(frames, minRange, maxRange);
}

void Player::SetupTransitions(void)
{
	// 状態の遷移条件を増加する
	// PLAYER_IDLE → PLAYER_CHANGE
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_CHANGE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsChange;
		});

	// PLAYER_IDLE → PLAYER_MOVE
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_MOVE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);	// 入った対象をPlayerになる
		return player->Input().moveHeld;
		});

	// PLAYER_IDLE → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsDash && player->OnFloor;
		});

	// PLAYER_IDLE → PLAYER_ATTACK
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_ATTACK], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsAttack;
		});

	// PLAYER_IDLE → PLAYER_SKILL
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_SKILL], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsSkill;
		});

	// PLAYER_IDLE → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_IDLE], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsHurt;
		});

	// PLAYER_MOVE → PLAYER_CHANGE
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_CHANGE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsChange;
		});

	// PLAYER_MOVE → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->Input().moveHeld;
		});

	// PLAYER_MOVE → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsDash && player->OnFloor;
		});

	// PLAYER_MOVE → PLAYER_ATTACK
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_ATTACK], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsAttack;
		});

	// PLAYER_MOVE → PLAYER_SKILL
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_SKILL], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsSkill;
		});

	// PLAYER_MOVE → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_MOVE], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsHurt;
		});

	// PLAYER_DASH → PLAYER_CHANGE
	FSM->AddTransition(PLAYER_STATE[PLAYER_DASH], PLAYER_STATE[PLAYER_CHANGE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsChange;
		});

	// PLAYER_DASH → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_DASH], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsDash;
		});

	// PLAYER_DASH → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_DASH], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsHurt;
		});

	// PLAYER_ATTACK → PLAYER_CHANGE
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_CHANGE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsChange;
		});

	// PLAYER_ATTACK → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsAttack;
		});

	// PLAYER_ATTACK → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsDash;
		});

	// PLAYER_ATTACK → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_ATTACK], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsFever && player->IsHurt;
		});

	// PLAYER_SKILL → PLAYER_CHANGE
	FSM->AddTransition(PLAYER_STATE[PLAYER_SKILL], PLAYER_STATE[PLAYER_CHANGE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsChange;
		});

	// PLAYER_SKILL → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_SKILL], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsSkill;
		});

	// PLAYER_SKILL → PLAYER_DASH
	FSM->AddTransition(PLAYER_STATE[PLAYER_SKILL], PLAYER_STATE[PLAYER_DASH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsDash;
		});

	// PLAYER_SKILL → PLAYER_HURT
	FSM->AddTransition(PLAYER_STATE[PLAYER_SKILL], PLAYER_STATE[PLAYER_HURT], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsHurt;
		});

	// PLAYER_HURT → PLAYER_CHANGE
	FSM->AddTransition(PLAYER_STATE[PLAYER_HURT], PLAYER_STATE[PLAYER_CHANGE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsChange && player->HpCurrent > 0;
		});

	// PLAYER_HURT → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_HURT], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsHurt && !player->IsDeath;
		});

	// PLAYER_HURT → PLAYER_DEATH
	FSM->AddTransition(PLAYER_STATE[PLAYER_HURT], PLAYER_STATE[PLAYER_DEATH], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return player->IsDeath;
		});

	// PLAYER_CHANGE → PLAYER_IDLE
	FSM->AddTransition(PLAYER_STATE[PLAYER_CHANGE], PLAYER_STATE[PLAYER_IDLE], [](Character* owner) {
		auto player = static_cast<Player*>(owner);
		return !player->IsChange;
		});
}

void Player::BuildInputSnapshot()
{
	auto& input = InputCodex::Get();

	inputSnap.ClearOneShots();
	inputSnap.padIndex = boundPadIndex;

	// -----------------------
	// Move
	float mx = 0.0f;
	float mz = 0.0f;
	// keyboard WASD
	if (input.KeyPressed(KK_A)) mx -= 1.0f;
	if (input.KeyPressed(KK_D)) mx += 1.0f;
	if (input.KeyPressed(KK_W)) mz += 1.0f;
	if (input.KeyPressed(KK_S)) mz -= 1.0f;
	// gamepad left stick
	if (boundPadIndex >= 0 && input.PadConnected(boundPadIndex))
	{
		mx += input.GP_LeftX(boundPadIndex);
		mz += input.GP_LeftY(boundPadIndex);
	}
	// gamepad cross key
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_LEFT))  mx -= 1.0f;
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_RIGHT)) mx += 1.0f;
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_UP))	 mz += 1.0f;
	if (input.GP_Pressed(boundPadIndex, Gamepad::GP_DPAD_DOWN))  mz -= 1.0f;
	// deadzone
	const float moveEps = 0.15f;
	if (fabsf(mx) < moveEps) mx = 0.0f;
	if (fabsf(mz) < moveEps) mz = 0.0f;
	Normalize2D(mx, mz);
	inputSnap.moveX = mx;
	inputSnap.moveZ = mz;
	inputSnap.moveHeld = (mx != 0.0f || mz != 0.0f);

	// -----------------------
	// Dash / Attack / Skill / SlotL / SlotR
	// dash: Space || Shift || Pad Y & B
	inputSnap.dash =
		input.KeyTriggered(KK_SPACE) || input.KeyTriggered(KK_LEFTSHIFT) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && (input.GP_Triggered(boundPadIndex, Gamepad::GP_Y) || input.GP_Triggered(boundPadIndex, Gamepad::GP_B)));
	// attack: MouseLeft || J || Pad X
	inputSnap.attack =
		input.MouseLeftTriggered() ||
		input.KeyTriggered(KK_J) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && input.GP_Triggered(boundPadIndex, Gamepad::GP_X));
	// skill: MouseRight || K || Pad A
	inputSnap.skill =
		input.MouseRightTriggered() ||
		input.KeyTriggered(KK_K) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && input.GP_Triggered(boundPadIndex, Gamepad::GP_A));
	// slotL: Q || Pad LB & LT
	inputSnap.slotL =
		input.KeyTriggered(KK_Q) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && (input.GP_Triggered(boundPadIndex, Gamepad::GP_LB) || input.GP_LT_Triggered(boundPadIndex)));
	// slotR: E || Pad RB & RT
	inputSnap.slotR =
		input.KeyTriggered(KK_E) ||
		(boundPadIndex >= 0 && input.PadConnected(boundPadIndex) && (input.GP_Triggered(boundPadIndex, Gamepad::GP_RB) || input.GP_RT_Triggered(boundPadIndex)));


	// -----------------------
	// Change / Fever
	if (GetWeaponSlots()->GetCanChange().first && !IsDeath)
	{
		bool kbCombo = input.KeyPressed(KK_J) && input.KeyPressed(KK_K);
		bool mouseCombo = input.MouseLeftPressed() && input.MouseRightPressed();
		bool padCombo = false;
		if (boundPadIndex >= 0 && input.PadConnected(boundPadIndex))
			padCombo = input.GP_Pressed(boundPadIndex, Gamepad::GP_X) && input.GP_Pressed(boundPadIndex, Gamepad::GP_A);

		if (mouseCombo || kbCombo || padCombo || (IsFever && !GetWeaponSlots()->GetIsFever()))
			inputSnap.change = true;
	}
}


/*------------------------------------------------------------------------------
   Player_IdleState
------------------------------------------------------------------------------*/
Player_IdleState::Player_IdleState()
{
	packs.emplace_back(12, 16, 145, 24);
	packs.emplace_back(12, 12,  49, 24);
	packs.emplace_back(12, 10,  97, 24);

	assist.Loop = true;
	assist.FPS = packs[0].FPS;
	assist.FrameTotal = packs[0].FrameTotalCount;
}

void Player_IdleState::OnEnter(Player* owner)
{
	assist.Reset();
}

void Player_IdleState::Update(Player* owner,  float dt)
{
	assist.Update(dt);

	// 遷移判断
	const auto& in = owner->Input();
	if (in.dash) owner->SetIsDash(true);
	if (in.attack)	owner->SetIsAttack(true);
	if (in.skill) if (owner->GetWeaponSlots()->GetCurrentSlot().GetType() != WEAPON_TYPE_0) owner->SetIsSkill(true);

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();

	// sprite set
	if (!owner->GetIsFever())
	{
		switch (owner->GetWeaponSlots()->GetCurrentSlot().GetType())
		{
		case WEAPON_TYPE_0:	owner->GetVisualPre()->SetFrame(packs[0].numU, packs[0].numV, packs[0].FrameStart, packs[0].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip()); break;
		case WEAPON_TYPE_1: owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 1, !owner->GetIsFlip()); break;
		case WEAPON_TYPE_2: owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 2, !owner->GetIsFlip()); break;
		case WEAPON_TYPE_3: owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 3, !owner->GetIsFlip()); break;
		default: break;
		}
	}
	else
	{
		owner->GetVisualPre()->SetFrame(packs[2].numU, packs[2].numV, packs[2].FrameStart, packs[2].FrameTotalCount, assist.FrameNo, 4, !owner->GetIsFlip());
	}
}


/*------------------------------------------------------------------------------
   Player_MoveState関数
------------------------------------------------------------------------------*/
Player_MoveState::Player_MoveState()
{
	packs.emplace_back(12, 16, 169, 24);
	packs.emplace_back(12, 12,  73, 24);
	packs.emplace_back(12, 10,  97, 24);

	assist.Loop = true;
	assist.FPS = packs[0].FPS;
	assist.FrameTotal = packs[0].FrameTotalCount;
}

void Player_MoveState::OnEnter(Player* owner)
{
	assist.Reset();
}

void Player_MoveState::Update(Player* owner, float dt)
{
	assist.Update(dt);

	// 遷移判断
	const auto& in = owner->Input();
	if (in.dash) owner->SetIsDash(true);
	if (in.attack)	owner->SetIsAttack(true);
	if (in.skill) if (owner->GetWeaponSlots()->GetCurrentSlot().GetType() != WEAPON_TYPE_0) owner->SetIsSkill(true);

	// 方向判断
	if (in.moveX < 0) owner->SetIsFlip(true);
	else owner->SetIsFlip(false);

	// 移動
	owner->DoMove(1.0f);

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();

	// sprite set
	if (!owner->GetIsFever())
	{
		switch (owner->GetWeaponSlots()->GetCurrentSlot().GetType())
		{
		case WEAPON_TYPE_0:	owner->GetVisualPre()->SetFrame(packs[0].numU, packs[0].numV, packs[0].FrameStart, packs[0].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip()); break;
		case WEAPON_TYPE_1: owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 1, !owner->GetIsFlip()); break;
		case WEAPON_TYPE_2: owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 2, !owner->GetIsFlip()); break;
		case WEAPON_TYPE_3: owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 3, !owner->GetIsFlip()); break;
		default: break;
		}
	}
	else
	{
		owner->GetVisualPre()->SetFrame(packs[2].numU, packs[2].numV, packs[2].FrameStart, packs[2].FrameTotalCount, assist.FrameNo, 4, !owner->GetIsFlip());
	}
}


/*------------------------------------------------------------------------------
   Player_DashState関数
------------------------------------------------------------------------------*/
void Player_DashState::OnEnter(Player* owner)
{
	// anime set
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, owner->GetMoveVelocity().x >= 0 ? true : false, false);

	// dash direction preparation
	const auto& in = owner->Input();
	if (in.moveHeld)
		isInput = true;
	else
		isInput = false;
}

void Player_DashState::Update(Player* owner, float dt)
{
	owner->GetVisualPre()->Update(dt);

	// デッシュ中に攻撃とスキルを事前入力する
	int currentFrame = owner->GetVisualPre()->GetCurrentFrame();

	if (pack.FrameTotalCount - currentFrame < 2)
	{
		if (!owner->GetIsAttack() && !owner->GetIsSkill())
		{
			const auto& in = owner->Input();
			if (in.attack) owner->SetIsAttack(true);
			if (in.skill) if (owner->GetWeaponSlots()->GetCurrentSlot().GetType() != WEAPON_TYPE_0) owner->SetIsSkill(true);
		}
	}

	// Dash発生の判断
	if (currentFrame == 1)
	{
		if (isInput)
		{
			XMFLOAT3 move = owner->GetMoveVelocity();
			float angle = atan2f(move.x, move.z);
			owner->CalculateMoveVelocity(sinf(angle) * owner->GetMoveAccel() * 40.0f, 0.0f, cosf(angle) * owner->GetMoveAccel() * 40.0f);
		}
		else
			owner->CalculateMoveVelocity((owner->GetIsFlip() ? -1.0f : 1.0f) * owner->GetMoveAccel() * 40.0f, 0.0f, 0.0f);
	}

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();

	// 終わったらデッシュフラグを閉じる
	if (owner->GetVisualPre()->ClipFinished())	owner->SetIsDash(false);
}


/*------------------------------------------------------------------------------
   Player_AttackState関数
------------------------------------------------------------------------------*/
Player_AttackState::Player_AttackState()
{
	packs.emplace_back(12, 16, 49, 15);
	packs.emplace_back(12, 12,  1, 15);
	packs.emplace_back(12, 10,  1, 12);
	ePacks.emplace_back( 7,  7, 1, 15);
	ePacks.emplace_back(12, 12, 1, 15);
	ePacks.emplace_back(12, 14, 1, 15);
	ePacks.emplace_back(12, 13, 1, 15);
	fPacks.emplace_back(9, 8,  1, 12);
	fPacks.emplace_back(9, 8, 25, 12);
	fPacks.emplace_back(9, 8, 13, 12);
	fPacks.emplace_back(9, 8, 37, 12);
	fPacks.emplace_back(9, 8, 49, 24);

	assist.Loop = false;
	assist.FPS = packs[0].FPS;
	assist.FrameTotal = packs[0].FrameTotalCount;
	oldFrame = 0;
}

void Player_AttackState::OnEnter(Player* owner)
{
	baseAttackCollisionSize = owner->GetAttackCollisionSize();

	// anime set
	assist.Reset();
	if (owner->GetFeverType() == WEAPON_TYPE_0)
	{
		auto type = owner->GetWeaponSlots()->GetCurrentSlot().GetType();
		if (type == WEAPON_TYPE_0)
		{
			mode = 0;
		}
		else
		{
			mode = 1;
		}
		packs[mode].FrameTotalCount = 15;
		owner->SetDoAttackType(Player_Attack_1);
		owner->GetWeaponSlots()->UseWeapon(-10.0f);

		SoundCodex::Get().PlaySE(SndPath::SE_Player_Attack_1);
	}
	else
	{
		mode = 2;
		packs[mode].FrameTotalCount = 12;
		owner->SetDoAttackType(Player_Fever_1);
	}

	assist.FrameTotal = packs[mode].FrameTotalCount;

	isCombo = false;
}

void Player_AttackState::OnExit(Player* owner)
{
	owner->ResetAttackCountDown();			// 攻撃終わったら攻撃のカウンターをリセットする
	owner->SetDoAttackType(Attack_Type_None);
	owner->SetAttackCollisionOnOff(false);	// もし攻撃の途中で攻撃されたら、こっちに攻撃フラグとコリジョンを閉じる
	owner->SetIsAttack(false);

	owner->SetAttackCollisionSize(baseAttackCollisionSize);
}

void Player_AttackState::Update(Player* owner, float dt)
{
	assist.Update(dt);

	auto type = owner->GetWeaponSlots()->GetCurrentSlot().GetType();
	auto attackType = owner->GetDoAttackType();

	// ダッシュで攻撃をキャンセル
	const auto& in = owner->Input();
	if (in.dash) owner->SetIsDash(true);
	// 攻撃中にスキルを事前入力する
	int currentFrame = assist.GetCurrentFrame();
	if (packs[mode].FrameTotalCount - currentFrame < 10)
	{
		if (in.skill)
			if (owner->GetWeaponSlots()->GetCurrentSlot().GetType() != WEAPON_TYPE_0)
				owner->SetIsSkill(true);
		if (!isCombo && attackType != Player_Attack_3)
		{
			if (in.attack)
			{
				isCombo = true;

				if (!owner->GetIsFever()) 
				{
					if (attackType == Player_Attack_1)
					{
						switch (type)
						{
						case WEAPON_TYPE_1:	owner->GetWeaponSlots()->UseWeapon(-30.0f); break;
						case WEAPON_TYPE_2:	owner->GetWeaponSlots()->UseWeapon(-20.0f); break;
						case WEAPON_TYPE_3: owner->GetWeaponSlots()->UseWeapon(-10.0f); break;
						}
					}
					if (attackType == Player_Attack_2)
					{
						switch (type)
						{
						case WEAPON_TYPE_1:	owner->GetWeaponSlots()->UseWeapon(-40.0f); break;
						case WEAPON_TYPE_2:	owner->GetWeaponSlots()->UseWeapon(-50.0f); break;
						case WEAPON_TYPE_3: owner->GetWeaponSlots()->UseWeapon(-10.0f); break;
						}
					}
				}
			}
		}
	}

	if (mode != 2)
	{
		switch (attackType)
		{
		case Player_Attack_1:
		{
			// 攻撃コリジョンを開けるフレーム
			if (currentFrame == 9 && oldFrame != currentFrame)
				owner->SetAttackCollisionOnOff(true);
			// 攻撃コリジョンを閉じるフレーム
			if (currentFrame == 14 && oldFrame != currentFrame)
				owner->SetAttackCollisionOnOff(false);
			// 攻撃当たる判定
			if (owner->GetAttackCollisionOnOff())
			{
				switch (type)
				{
				case WEAPON_TYPE_0:	owner->AttackCollide(-1.0f, { 0.2f,0.1f,0.2f }); break;
				case WEAPON_TYPE_1:	owner->AttackCollide(-2.0f, { 0.3f,0.2f,0.3f }); break;
				case WEAPON_TYPE_3: owner->AttackCollide(-1.0f, { 0.1f,0.1f,0.1f }); break;
				}
			}
			break;
		}
		case Player_Attack_2:
		{
			// 攻撃コリジョンを開けるフレーム
			if (currentFrame == 25 && oldFrame != currentFrame)	owner->SetAttackCollisionOnOff(true);
			// 攻撃コリジョンを閉じるフレーム
			if (currentFrame == 30 && oldFrame != currentFrame)	owner->SetAttackCollisionOnOff(false);
			// 攻撃当たる判定
			if (owner->GetAttackCollisionOnOff()) 
			{
				switch (type)
				{
				case WEAPON_TYPE_0:	owner->AttackCollide(-1.0f, { 0.1f,0.2f,0.1f }); break;
				case WEAPON_TYPE_1:	owner->AttackCollide(-2.0f, { 0.2f,0.3f,0.2f }); break;
				case WEAPON_TYPE_3: owner->AttackCollide(-1.0f, { 0.1f,0.1f,0.1f }); break;
				}
			}
			break;
		}
		case Player_Attack_3:
		{
			// 攻撃コリジョンを開けるフレーム
			if (currentFrame == 36 && oldFrame != currentFrame)
				owner->SetAttackCollisionOnOff(true);
			// 攻撃コリジョンを閉じるフレーム
			if (currentFrame == 41 && oldFrame != currentFrame)
				owner->SetAttackCollisionOnOff(false);
			// 攻撃当たる判定
			if (owner->GetAttackCollisionOnOff())
			{
				switch (type)
				{
				case WEAPON_TYPE_0:	owner->AttackCollide(-2.0f, { 0.3f,0.1f,0.3f }); break;
				case WEAPON_TYPE_1:	owner->AttackCollide(-2.0f, { 0.4f,0.2f,0.4f }); break;
				case WEAPON_TYPE_3: owner->AttackCollide(-1.0f, { 0.1f,0.1f,0.1f }); break;
				}
			}
			break;
		}
		}
	}
	else
	{
		switch (attackType)
		{
		case Player_Fever_1:
		{
			// 攻撃コリジョンを開けるフレーム
			if (currentFrame == 9 && oldFrame != currentFrame)
			{
				owner->SetAttackCollisionSize({ 6.0f,6.0f,6.0f });
				owner->SetAttackCollisionOnOff(true);
			}
			// 攻撃コリジョンを閉じるフレーム
			if (currentFrame == 11 && oldFrame != currentFrame)
				owner->SetAttackCollisionOnOff(false);
			// 攻撃当たる判定
			if (owner->GetAttackCollisionOnOff()) owner->AttackCollide(-2.0f, { 0.3f,0.3f,0.3f });
			break;
		}
		case Player_Fever_2:
		{
			// 攻撃コリジョンを開けるフレーム
			if (currentFrame == 17 && oldFrame != currentFrame)	
			{
				owner->SetAttackCollisionSize({ 8.0f,16.0f,8.0f });
				owner->SetAttackCollisionOnOff(true);
			}
			// 攻撃コリジョンを閉じるフレーム
			if (currentFrame == 19 && oldFrame != currentFrame)	
				owner->SetAttackCollisionOnOff(false);
			// 攻撃当たる判定
			if (owner->GetAttackCollisionOnOff()) owner->AttackCollide(-3.0f, { 0.5f,0.5f,0.5f });
			break;
		}
		case Player_Fever_3:
		{
			// 攻撃コリジョンを開けるフレーム
			if (currentFrame == 40 && oldFrame != currentFrame)
			{
				owner->SetAttackCollisionSize({ 30.0f,10.0f,30.0f });
				owner->SetAttackCollisionOnOff(true);
			}
			// 攻撃コリジョンを閉じるフレーム
			if (currentFrame == 46 && oldFrame != currentFrame)
				owner->SetAttackCollisionOnOff(false);
			// 攻撃当たる判定
			if (owner->GetAttackCollisionOnOff()) owner->AttackCollide(-5.0f, { 1.5f ,0.5f,1.5f });
			break;
		}
		}
	}

	// 連撃の判断
	if (currentFrame == packs[mode].FrameTotalCount && isCombo)
	{
		// もし連撃したら、スキルの事前入力を消ます
		owner->SetIsSkill(false);
		owner->SetDoAttackType(static_cast<Attack_Type_Tag>(attackType + 1));

		if (mode != 2)
		{
			if (owner->GetDoAttackType() == Player_Attack_2)
			{
				packs[mode].FrameTotalCount = 32;
				SoundCodex::Get().PlaySE(SndPath::SE_Player_Attack_2);
			}
			else
			{
				packs[mode].FrameTotalCount = 48;
				SoundCodex::Get().PlaySE(SndPath::SE_Player_Attack_3);
			}
		}
		else
			packs[mode].FrameTotalCount = (owner->GetDoAttackType() == Player_Fever_2) ? 24 : 48;
		assist.FrameTotal = packs[mode].FrameTotalCount;

		isCombo = false;
	}

	// 移動
	owner->DoMove(0.1f);

	// sprite set
	if (mode != 2)
	{
		switch (owner->GetWeaponSlots()->GetCurrentSlot().GetType())
		{
		case WEAPON_TYPE_0:	
				owner->GetVisualPre()->SetFrame(packs[0].numU, packs[0].numV, packs[0].FrameStart, packs[0].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip()); 
				owner->GetVisualPreEffect()->SetFrame(ePacks[0].numU, ePacks[0].numV, ePacks[0].FrameStart, packs[0].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip());
				break;
		case WEAPON_TYPE_1: 
				owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 1, !owner->GetIsFlip()); 
				owner->GetVisualPreEffect()->SetFrame(ePacks[1].numU, ePacks[1].numV, ePacks[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 1, !owner->GetIsFlip());
				break;
		case WEAPON_TYPE_2: 
				owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 2, !owner->GetIsFlip()); 
				//owner->GetVisualPreEffectAttack()->SetFrame(ePacks[2].numU, ePacks[2].numV, ePacks[2].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 2, !owner->GetIsFlip());
				break;
		case WEAPON_TYPE_3: 
				owner->GetVisualPre()->SetFrame(packs[1].numU, packs[1].numV, packs[1].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 3, !owner->GetIsFlip()); 
				owner->GetVisualPreEffect()->SetFrame(ePacks[3].numU, ePacks[3].numV, ePacks[3].FrameStart, packs[1].FrameTotalCount, assist.FrameNo, 3, !owner->GetIsFlip());
				break;
		}
	}
	else
	{
		owner->GetVisualPre()->SetFrame(packs[2].numU, packs[2].numV, packs[2].FrameStart, packs[2].FrameTotalCount, assist.FrameNo, 4, !owner->GetIsFlip());

		auto wf = owner->GetVisualPreFever();
		switch (attackType)
		{
		case Player_Fever_1: 
			wf[0]->SetFrame(fPacks[0].numU, fPacks[0].numV, fPacks[0].FrameStart, fPacks[0].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip());
			wf[1]->SetFrame(fPacks[1].numU, fPacks[1].numV, fPacks[1].FrameStart, fPacks[1].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip());
			break;
		case Player_Fever_2: 
			wf[0]->SetFrame(fPacks[2].numU, fPacks[2].numV, fPacks[2].FrameStart, fPacks[2].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip());
			wf[1]->SetFrame(fPacks[3].numU, fPacks[3].numV, fPacks[3].FrameStart, fPacks[3].FrameTotalCount, assist.FrameNo, 0, !owner->GetIsFlip());
			break;
		case Player_Fever_3: 
			owner->SetAttackPosition(owner->GetPosition());
			for (int i = 0;i < wf.size();i++) 
				wf[i]->SetFrame(fPacks[4].numU, fPacks[4].numV, fPacks[4].FrameStart, fPacks[4].FrameTotalCount, assist.FrameNo, 0, (i < 3) ? true : false);
			break;
		}
	}

	// save old frame
	oldFrame = currentFrame;

	// close the attack flag when the attack is done
	if (assist.Finished())	owner->SetIsAttack(false);
}


/*------------------------------------------------------------------------------
   Player_SkillState
------------------------------------------------------------------------------*/
Player_SkillState::Player_SkillState()
{
	ePacks.emplace_back(12, 12, 49, 48);
	ePacks.emplace_back(12, 14, 49, 48);
	ePacks.emplace_back(12, 13, 49, 48);
	bgPacks.emplace_back(12, 12, 97, 48);
	bgPacks.emplace_back(12, 14, 97, 48);
	bgPacks.emplace_back(12, 13, 97, 48);
}

void Player_SkillState::OnEnter(Player* owner)
{
	baseAttackCollisionSize = owner->GetAttackCollisionSize();

	// anime set
	WeaponType = owner->GetWeaponSlots()->GetCurrentSlot().GetType();
	switch (WeaponType)
	{
	case WEAPON_TYPE_1: 
		owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 1, pack.FPS, !owner->GetIsFlip(), false);
		owner->GetVisualPreEffectBg()->SetFrameAuto(bgPacks[0].numU, bgPacks[0].numV, bgPacks[0].FrameStart, bgPacks[0].FrameTotalCount, 4, bgPacks[0].FPS, !owner->GetIsFlip(), false);
		owner->GetVisualPreEffect()->SetFrameAuto(ePacks[0].numU, ePacks[0].numV, ePacks[0].FrameStart, ePacks[0].FrameTotalCount, 1, ePacks[0].FPS, !owner->GetIsFlip(), false);
		break;
	case WEAPON_TYPE_2: 
		owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 2, pack.FPS, !owner->GetIsFlip(), false);
		owner->GetVisualPreEffectBg()->SetFrameAuto(bgPacks[1].numU, bgPacks[1].numV, bgPacks[1].FrameStart, bgPacks[1].FrameTotalCount, 5, bgPacks[1].FPS, !owner->GetIsFlip(), false);
		owner->GetVisualPreEffect()->SetFrameAuto(ePacks[1].numU, ePacks[1].numV, ePacks[1].FrameStart, ePacks[1].FrameTotalCount, 2, ePacks[1].FPS, !owner->GetIsFlip(), false);
		break;
	case WEAPON_TYPE_3: 
		owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 3, pack.FPS, !owner->GetIsFlip(), false);
		owner->GetVisualPreEffectBg()->SetFrameAuto(bgPacks[2].numU, bgPacks[2].numV, bgPacks[2].FrameStart, bgPacks[2].FrameTotalCount, 6, bgPacks[2].FPS, !owner->GetIsFlip(), false);
		owner->GetVisualPreEffect()->SetFrameAuto(ePacks[2].numU, ePacks[2].numV, ePacks[2].FrameStart, ePacks[2].FrameTotalCount, 3, ePacks[2].FPS, !owner->GetIsFlip(), false);
		break;
	}

	// skill set
	switch (WeaponType)
	{
	case WEAPON_TYPE_1:
		owner->SetAttackCollisionSize({ 20.0f,5.0f,20.0f });
		owner->GetVisualPreEffectBg()->SetScale(20.0f, 20.0f, 0.0f);
		owner->GetVisualPreEffect()->SetScale(20.0f, 20.0f, 0.0f);
		owner->SetDoAttackType(Player_Skill_1);
		break;
	case WEAPON_TYPE_2:
		owner->GetVisualPreEffectBg()->SetScale(7.5f, 7.5f, 0.0f);
		owner->GetVisualPreEffect()->SetScale(7.5, 7.5f, 0.0f);
		owner->SetDoAttackType(Player_Skill_2);
		break;
	case WEAPON_TYPE_3:
		owner->SetAttackCollisionSize({ 5.0f,5.0f,5.0f });
		owner->GetVisualPreEffectBg()->SetScale(10.0f, 10.0f, 0.0f);
		owner->GetVisualPreEffect()->SetScale(10.0f, 10.0f, 0.0f);
		owner->SetDoAttackType(Player_Skill_3);
		break;
	}
	owner->GetWeaponSlots()->UseWeapon(-100.0f);

	owner->SetCollisionOnOff(false);	// コリジュンを閉じる
	oldFrame = 0;
}

void Player_SkillState::OnExit(Player* owner)
{
	switch (WeaponType)
	{
	case WEAPON_TYPE_1:
	{
		owner->SetAttackCollisionSize(baseAttackCollisionSize);
		owner->GetVisualPreEffectBg()->SetScale(owner->GetEffectSize());
		owner->GetVisualPreEffect()->SetScale(owner->GetEffectSize());
		break;
	}
	case WEAPON_TYPE_2:
	{
		owner->GetVisualPreEffectBg()->SetScale(owner->GetEffectSize());
		owner->GetVisualPreEffect()->SetScale(owner->GetEffectSize());
		break;
	}
	case WEAPON_TYPE_3:
	{
		owner->SetAttackCollisionSize(baseAttackCollisionSize);
		owner->GetVisualPreEffectBg()->SetScale(owner->GetEffectSize());
		owner->GetVisualPreEffect()->SetScale(owner->GetEffectSize());
		break;
	}
	}

	owner->SetDoAttackType(Attack_Type_None);
	owner->SetCollisionOnOff(true); // コリジュンを開ける
	owner->SetIsSkill(false);
	owner->SetWasSkill(true);
}

void Player_SkillState::Update(Player* owner, float dt)
{
	// set anime
	owner->GetVisualPre()->Update(dt);
	owner->GetVisualPreEffect()->Update(dt);

	// ダッシュでスキルをキャンセル
	const auto& in = owner->Input();
	if (in.dash) owner->SetIsDash(true);
	// スキル中に攻撃とスキルを事前入力する
	int currentFrame = owner->GetVisualPre()->GetCurrentFrame();

	if (pack.FrameTotalCount - currentFrame < 2)
	{
		if (!owner->GetIsAttack() && !owner->GetIsSkill())
		{
			if (in.attack) owner->SetIsAttack(true);
			if (in.skill) if (owner->GetWeaponSlots()->GetCurrentSlot().GetType() != WEAPON_TYPE_0) owner->SetIsSkill(true);
		}
	}

	auto pos = owner->GetPosition();
	switch (WeaponType)
	{
	case WEAPON_TYPE_1:
	{
		owner->SetAttackPosition(owner->GetPosition());
		// 攻撃コリジョンを開けるフレーム
		if (currentFrame == 31 && oldFrame != currentFrame) 
		{
			owner->SetAttackCollisionOnOff(true);
			SoundCodex::Get().PlaySE(SndPath::SE_Player_Skill_Red);
		}
		// 攻撃コリジョンを閉じるフレーム
		if (currentFrame == 46 && oldFrame != currentFrame) owner->SetAttackCollisionOnOff(false);
		// 攻撃当たる判定
		if (owner->GetAttackCollisionOnOff()) 
			owner->AttackCollide(-3.0f, { 1.0f,0.5f,1.0f });
		// VisualPrePos
		owner->GetVisualPreEffectBg()->SetPosition(pos.x, pos.y + 5.0f, pos.z + 0.1f);
		owner->GetVisualPreEffect()->SetPosition(pos.x, pos.y + 5.0f, pos.z - 0.1f);
		break;
	}
	case WEAPON_TYPE_2:
		if (currentFrame == 45 && oldFrame != currentFrame) 
		{
			owner->CalculateHpCurrent(10.0f);
			SoundCodex::Get().PlaySE(SndPath::SE_Player_Skill_Green);
		}
		// VisualPrePos
		owner->GetVisualPreEffectBg()->SetPosition(pos.x, pos.y + 1.25f, pos.z + 0.1f);
		owner->GetVisualPreEffect()->SetPosition(pos.x, pos.y + 1.25f, pos.z - 0.1f);
		break;
	case WEAPON_TYPE_3:
		// 攻撃コリジョンを開けるフレーム
		if (currentFrame == 17 && oldFrame != currentFrame)
		{
			owner->SetAttackCollisionOnOff(true);
			owner->CalculateMoveVelocity((owner->GetIsFlip() ? -1.0f : 1.0f) * owner->GetMoveAccel() * 60.0f, 0.1f, 0.0f);
			SoundCodex::Get().PlaySE(SndPath::SE_Player_Skill_Blue);
		}
		// 攻撃コリジョンを閉じるフレーム
		if (currentFrame == 43 && oldFrame != currentFrame) owner->SetAttackCollisionOnOff(false);
		// 攻撃当たる判定
		if (owner->GetAttackCollisionOnOff()) owner->AttackCollide(-1.0f, { 0.5f,0.0f,1.0f });
		// VisualPrePos
		owner->GetVisualPreEffectBg()->SetPosition(pos.x, pos.y, pos.z + 0.1f);
		owner->GetVisualPreEffect()->SetPosition(pos.x, pos.y, pos.z - 0.1f);
		break;
	}

	// 移動
	owner->DoMove(0.1f);

	// 終わったらスキルフラグを閉じる
	if (owner->GetVisualPre()->ClipFinished()) owner->SetIsSkill(false);

	// save old frame
	oldFrame = currentFrame;
}


/*------------------------------------------------------------------------------
   Player_HurtState
------------------------------------------------------------------------------*/
void Player_HurtState::OnEnter(Player* owner)
{
	// anime set
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);

	owner->SetCollisionOnOff(false); // コリジュンを閉じる

	owner->GetResourceBars()->ResetCombo();

	SoundCodex::Get().PlaySE(SndPath::SE_Player_Hurt);
	InputCodex::Get().GP_SetVibrationPulse(owner->boundPadIndex, 0.8f, 0.8f, 30);
}

void Player_HurtState::OnExit(Player* owner)
{
	owner->SetIsHurt(false); // DeathStateに遷移するかも、も一回IsHurtをリセットする
	owner->SetCollisionOnOff(true); // コリジュンを開ける
}

void Player_HurtState::Update(Player* owner, float dt)
{
	owner->GetVisualPre()->Update(dt);

	// 攻撃されたの中にデッシュと攻撃とスキルを事前入力する
	if (pack.FrameTotalCount - owner->GetVisualPre()->GetCurrentFrame() < 2)
	{
		if (!owner->GetIsDash() && !owner->GetIsAttack() && !owner->GetIsSkill())
		{
			const auto& in = owner->Input();
			if (in.dash) owner->SetIsDash(true);
			if (in.attack) owner->SetIsAttack(true);
			if (in.skill) if (owner->GetWeaponSlots()->GetCurrentSlot().GetType() != WEAPON_TYPE_0) owner->SetIsSkill(true);
		}
	}

	// 攻撃をカウントダウン
	owner->DoAttackCountDown();

	// 攻撃された後は何するの判断
	if (owner->GetVisualPre()->ClipFinished())
	{
		// もし体力がゼロたら、死ぬ状態で遷移
		if (owner->GetHpCurrent() <= 0)	owner->SetIsDeath(true);
		// 終わったら攻撃されたフラグを閉じる
		else owner->SetIsHurt(false);
	}
}


/*------------------------------------------------------------------------------
   Player_DeathState
------------------------------------------------------------------------------*/
void Player_DeathState::OnEnter(Player* owner)
{
	// anime set
	owner->GetVisualPre()->SetFrameAuto(pack.numU, pack.numV, pack.FrameStart, pack.FrameTotalCount, 0, pack.FPS, !owner->GetIsFlip(), false);
}

void Player_DeathState::Update(Player* owner, float dt)
{
	owner->GetVisualPre()->Update(dt);
		
	// 終わったら何がする…
	if (owner->GetVisualPre()->ClipFinished())	owner->SetIsGameOver(true);
}


/*------------------------------------------------------------------------------
   Player_ChangeState
------------------------------------------------------------------------------*/
void Player_ChangeState::OnEnter(Player* owner)
{
	// anime set
	if (owner->GetIsFever())
	{
		owner->SetFeverType(WEAPON_TYPE_0);

		owner->GetVisualPre()->SetFrameAuto(packOut.numU, packOut.numV, packOut.FrameStart, packOut.FrameTotalCount, 4, packOut.FPS, !owner->GetIsFlip(), false);

		// Blink
		owner->GetVisualPreFeverMega()->SetFrameAuto(5, 3, 1, 12, 0, packOut.FPS, !owner->GetIsFlip(), false);
		float offset = owner->GetIsFlip() ? -100.0f : 100.0f;
		owner->GetVisualPreFeverMega()->SetPosition(SCREEN_WIDTH / 2.0f + offset, SCREEN_HEIGHT / 2.0f);

		oldFrame = 0;
	}
	else
	{
		owner->SetFeverType(owner->GetWeaponSlots()->GetCanChange().second);
		owner->GetWeaponSlots()->DoChange();

		owner->GetVisualPreEffect()->SetFrameAuto(packIn.numU, packIn.numV, 1, 25, 5, packIn.FPS, false, true);
		owner->GetVisualPreEffectBg()->SetFrameAuto(packIn.numU, packIn.numV, 24, 23, 8, packIn.FPS, false, true);
	}

	owner->SetIsFever(!owner->GetIsFever());
}

void Player_ChangeState::OnExit(Player* owner)
{
	owner->SetIsDash(false);
	owner->SetIsAttack(false);
	owner->SetIsSkill(false);
	owner->SetIsHurt(false);

	if (owner->GetIsFever())
	{
		owner->GetVisualPreEffectBg()->SetFrameAuto(12, 12, 1, 144, 7, 20.0f, false, true);
		owner->GetVisualPreEffect()->SetFrameAuto(12, 12, 1, 144, 4, 20.0f, false, true);
	}
	else
	{
		owner->SetBackground();
		owner->SetAttackCollisionSize({ 4.0f,6.0f,2.5f });
		owner->SetAttackCollisionOnOff(false);
		owner->SetDoAttackType(Attack_Type_None);
	}

	owner->GetWeaponSlots()->FinishChange();
}

void Player_ChangeState::Update(Player* owner, float dt)
{
	// anime set
	if (owner->GetIsFever())
	{
		owner->GetVisualPreEffect()->Update(dt);

		//変身完了の判断
		if (owner->GetVisualPreEffectBg()->ClipFinished())
			owner->SetIsChange(false);
	}
	else
	{
		int currentFrame = owner->GetVisualPre()->GetCurrentFrame();

		owner->GetVisualPre()->Update(dt);

		if (currentFrame >= 24 && oldFrame != currentFrame) owner->SetIsFeverMega(true);
		// Attack
		if (owner->GetVisualPreFeverMega()->ClipFinished())
		{
			owner->SetAttackCollisionSize({ 500.0f, 500.0f, 500.0f });
			owner->SetAttackCollisionOnOff(true);
			owner->SetDoAttackType(Player_Fever_Mega);
		}
		if (owner->GetAttackCollisionOnOff()) owner->AttackCollide(-100.0f, { 1.0f,0.1f,1.0f });
		// Mega
		if (currentFrame >= 36 && oldFrame != currentFrame && owner->GetVisualPreFeverMega()->ClipFinished())
			owner->GetVisualPreFeverMega()->SetFrameAuto(6, 3, 1, 18, 1, packOut.FPS, !owner->GetIsFlip(), false);

		//変身完了の判断
		if (owner->GetVisualPre()->ClipFinished())
			owner->SetIsChange(false);

		oldFrame = currentFrame;
	}
}