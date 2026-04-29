#pragma once
#include "Effect.h"
#include "Enemy.h"
#include "RenderGraph.h"
#include "Sprite3DNoLit.h"
#include "AnimationInfo.h"
#include "Channels.h"

class Effect_Player_Remote_2 : public Effect
{
public:
	Effect_Player_Remote_2(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, bool flip, Object_Type_Tag tag = effect_Player_Remote_Attack_2)
		:
		Effect(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 5.0f,5.0f,0.0f });
		SetCollisionSize({ 5.0f,5.0f,1.0f });
		SetCollisionOnOff(true);
		SetMoveAccel((flip ? -1.0f : 1.0f) * speedNoPlay);
		SetDoAttackType(Player_Attack_2);
		SetEffectState(Appear);
		lastTime = 0.0f;

		// graphics init
		pack.push_back(SpriteAnimeInfo{ 12,14, 25,18 });
		pack.push_back(SpriteAnimeInfo{ 12,14, 40, 3 });
		pack.push_back(SpriteAnimeInfo{ 12,14, 43, 6 });
		pack.push_back(SpriteAnimeInfo{ 12,14,145,16 });
		visualPre = std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{"asset\\Images\\\Player\\Player_Green_Effect.png"});
		visualPre->SetFrameAuto(pack[0].numU, pack[0].numV, pack[0].FrameStart, pack[0].FrameTotalCount, 0, pack[0].FPS, false, false);
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->LinkTechniques(rg);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(0.0f, 0.0f, 1.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void SpawnAt(XMFLOAT3 pos, bool flip) override
	{
		// reset parameters
		SetPosition(pos);
		SetCollisionOnOff(true);
		boxCollider.center = transInfo.position;
		SetMoveAccel((flip ? -1.0f : 1.0f) * speedNoPlay);
		SetEffectState(Appear);
		ResetMoveVelocity();
		lastTime = 0.0f;

		// reset animation
		visualPre->SetFrameAuto(pack[0].numU, pack[0].numV, pack[0].FrameStart, pack[0].FrameTotalCount, 0, pack[0].FPS, false, false);
		visualPre->SetPosition(transInfo.position);
	}
	void OnEnable(void) override {}
	void Update(float dt) override
	{
		// anime update
		visualPre->Update(dt);
		switch (effectState)
		{
		case Appear:
			if (visualPre->ClipFinished())
			{
				SetEffectState(Play);
				SetMoveAccel(GetMoveAccel() / speedNoPlay * speedPlay);
				visualPre->SetFrameAuto(pack[1].numU, pack[1].numV, pack[1].FrameStart, pack[1].FrameTotalCount, 0, pack[1].FPS, false, true);
			}
			break;
		case Play:
			lastTime += dt;
			if (lastTime >= lifeTime && visualPre->GetCurrentFrame() == 3)
			{
				SetEffectState(Disappear);
				SetMoveAccel(0.0);
				MoveVelocity = { 0.0f,0.0f,0.0f };
				SetCollisionOnOff(false);
				visualPre->SetFrameAuto(pack[2].numU, pack[2].numV, pack[2].FrameStart, pack[2].FrameTotalCount, 0, pack[2].FPS, false, false);
			}
			break;
		case Disappear:
			if (visualPre->ClipFinished())
			{
				Deactivate();
			}
			break;
		}

		// move update
		CalculateMoveVelocity(GetMoveAccel(), 0.0f, 0.0f);
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		visualPre->SetPosition(transInfo.position);

		// collider update
		boxCollider.center = transInfo.position;
	}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
#ifdef _DEBUG
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
#endif
	}
	void OnCollide(Character* other) override
	{
		SetEffectState(Disappear);
		SetMoveAccel(0.0);
		MoveVelocity = { 0.0f,0.0f,0.0f };
		SetCollisionOnOff(false);
		transInfo.position = other->GetPosition();
		transInfo.position.z -= 0.1f;

		visualPre->SetFrameAuto(pack[3].numU, pack[3].numV, pack[3].FrameStart, pack[3].FrameTotalCount, 0, pack[3].FPS, false, true);

		if (auto* e = dynamic_cast<Enemy*>(other))
		{
			if (e->GetBeAttackedType() != GetDoAttackType())
			{
				e->SetIsHurt(true);	// çUåÇÇ≥ÇÍÇΩèÛë‘Ç…ëJà⁄
				e->SetWasHurt(true);
				e->CalculateHpCurrent(-2.0f); // ëÃóÕåvéZ
				e->SetBeAttackedType(GetDoAttackType());

				float dx = e->GetPosition().x - transInfo.position.x;
				float dz = e->GetPosition().z - transInfo.position.z;
				float angle = atan2f(dx, dz);
				XMFLOAT3 repel{ 0.3f,0.2f,0.1f };
				other->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z); // åÇëﬁÇ∑ÇÈ

				// ÉQÅ[ÉÄÉäÉ\Å[ÉXîªíf(FeverÇ∂Ç·Ç»Ç¢éûÇæÇØÉGÉlÉãÉMÅ[Ç™Ç‡ÇÁÇ¶ÇÈ)
				auto p = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
				p->GetResourceBars()->AttackSettlement(e->GetEnemyType(), p->GetIsFever());
				p->AttackCameraShake(4, 0.5f, 1.0f);
			}
		}
	}
private:
	std::unique_ptr<Sprite3DNoLit> visualPre;
	std::vector<SpriteAnimeInfo> pack;
	static constexpr float speedNoPlay = 0.003f;
	static constexpr float speedPlay = speedNoPlay * 10.0f;
#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> searchColliderWire;
#endif
};