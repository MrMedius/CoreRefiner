#pragma once
#include "Effect.h"
#include "Enemy.h"
#include "RenderGraph.h"
#include "Sprite3DNoLit.h"
#include "AnimationInfo.h"
#include "Channels.h"

class Effect_Player_Remote_3 : public Effect
{
public:
	Effect_Player_Remote_3(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, bool flip, Object_Type_Tag tag = effect_Player_Remote_Attack_3)
		:
		Effect(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 7.0f,7.0f,0.0f });
		SetCollisionSize({ 7.0f,7.0f,7.0f });
		SetCollisionOnOff(true);
		SetMoveAccel(0.005f);
		SetMoveAccel((flip ? -1.0f : 1.0f) * speedNoPlay);
		SetDoAttackType(Player_Attack_3);
		SetEffectState(Appear);
		lifeTime = 1.0f;
		lastTime = 0.0f;

		// graphics init
		pack.push_back(SpriteAnimeInfo{ 12,14, 25,18 });
		pack.push_back(SpriteAnimeInfo{ 12,14, 40, 3 });
		pack.push_back(SpriteAnimeInfo{ 12,14, 43, 6 });
		pack.push_back(SpriteAnimeInfo{ 12,14,145,16 });
		visualPre_1 = std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{"asset\\Images\\\Player\\Player_Green_Effect.png"});
		visualPre_1->SetFrameAuto(pack[0].numU, pack[0].numV, pack[0].FrameStart, pack[0].FrameTotalCount, 0, pack[0].FPS, false, false);
		visualPre_1->SetPosition(transInfo.position);
		visualPre_1->SetScale(transInfo.scale);
		visualPre_1->LinkTechniques(rg);
		visualPre_2 = std::make_unique<Sprite3DNoLit>(gfx, std::vector<std::string>{"asset\\Images\\\Player\\Player_Green_Effect.png"});
		visualPre_2->SetFrameAuto(pack[0].numU, pack[0].numV, pack[0].FrameStart, pack[0].FrameTotalCount, 0, pack[0].FPS, false, false);
		visualPre_2->SetPosition(transInfo.position);
		visualPre_2->SetScale(transInfo.scale);
		visualPre_2->LinkTechniques(rg);

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
		visualPre_1->SetFrameAuto(pack[0].numU, pack[0].numV, pack[0].FrameStart, pack[0].FrameTotalCount, 0, pack[0].FPS, false, false);
		visualPre_1->SetPosition(transInfo.position);
		visualPre_2->SetFrameAuto(pack[0].numU, pack[0].numV, pack[0].FrameStart, pack[0].FrameTotalCount, 0, pack[0].FPS, false, false);
		visualPre_2->SetPosition(transInfo.position);
	}
	void OnEnable(void) override {}
	void Update(float dt) override
	{
		// anime update
		visualPre_1->Update(dt);
		visualPre_2->Update(dt);
		switch (effectState)
		{
		case Appear:
			if (visualPre_1->ClipFinished())
			{
				SetEffectState(Play);
				SetMoveAccel(GetMoveAccel() / speedNoPlay * speedPlay);
				visualPre_1->SetFrameAuto(pack[1].numU, pack[1].numV, pack[1].FrameStart, pack[1].FrameTotalCount, 0, pack[1].FPS, false, true);
				visualPre_2->SetFrameAuto(pack[1].numU, pack[1].numV, pack[1].FrameStart, pack[1].FrameTotalCount, 0, pack[1].FPS, false, true);
			}
			break;
		case Play:
			lastTime += dt;
			if (lastTime >= lifeTime && visualPre_1->GetCurrentFrame() == 3)
			{
				SetEffectState(Disappear);
				SetMoveAccel(0.0);
				MoveVelocity = { 0.0f,0.0f,0.0f };
				SetCollisionOnOff(false);
				visualPre_1->SetFrameAuto(pack[2].numU, pack[2].numV, pack[2].FrameStart, pack[2].FrameTotalCount, 0, pack[2].FPS, false, false);
				visualPre_2->SetFrameAuto(pack[2].numU, pack[2].numV, pack[2].FrameStart, pack[2].FrameTotalCount, 0, pack[2].FPS, false, false);
			}
			break;
		case Disappear:
			if (visualPre_1->ClipFinished())
			{
				Deactivate();
			}
			break;
		}

		// move update
		CalculateMoveVelocity(GetMoveAccel(), 0.0f, 0.0f);
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		float r = dt * 300.0f;
		Rotate(r, r, r);
		visualPre_1->SetPosition(transInfo.position);
		visualPre_1->SetRotation(transInfo.rotation);
		visualPre_2->SetPosition(transInfo.position);
		visualPre_2->SetRotation(-transInfo.rotation.x, -transInfo.rotation.y, -transInfo.rotation.z);

		// collider update
		boxCollider.center = transInfo.position;
	}
	void Submit(void) override
	{
		visualPre_1->Submit(Chan::main);
		visualPre_2->Submit(Chan::main);
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

		visualPre_1->SetFrameAuto(pack[3].numU, pack[3].numV, pack[3].FrameStart, pack[3].FrameTotalCount, 0, pack[3].FPS, false, true);
		visualPre_2->SetFrameAuto(pack[3].numU, pack[3].numV, pack[3].FrameStart, pack[3].FrameTotalCount, 0, pack[3].FPS, false, true);

		if (auto* e = dynamic_cast<Enemy*>(other))
		{
			if (e->GetBeAttackedType() != GetDoAttackType())
			{
				e->SetIsHurt(true);	// çUåÇÇ≥ÇÍÇΩèÛë‘Ç…ëJà⁄
				e->SetWasHurt(true);
				e->CalculateHpCurrent(-3.0f); // ëÃóÕåvéZ
				e->SetBeAttackedType(GetDoAttackType());

				float dx = e->GetPosition().x - transInfo.position.x;
				float dz = e->GetPosition().z - transInfo.position.z;
				float angle = atan2f(dx, dz);
				XMFLOAT3 repel{ 0.3f,0.3f,0.3f };
				other->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z); // åÇëﬁÇ∑ÇÈ

				// ÉQÅ[ÉÄÉäÉ\Å[ÉXîªíf(FeverÇ∂Ç·Ç»Ç¢éûÇæÇØÉGÉlÉãÉMÅ[Ç™Ç‡ÇÁÇ¶ÇÈ)
				auto p = ObjectCodex::FindFirstActiveObjectByTag<Player>(character_Player);
				p->GetResourceBars()->AttackSettlement(e->GetEnemyType(), p->GetIsFever());
				p->AttackCameraShake(6, 1.0f, 1.5f);
			}
		}
	}
private:
	std::unique_ptr<Sprite3DNoLit> visualPre_1;
	std::unique_ptr<Sprite3DNoLit> visualPre_2;
	std::vector<SpriteAnimeInfo> pack;
	static constexpr float speedNoPlay = 0.001f;
	static constexpr float speedPlay = speedNoPlay * 10.0f;
#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> searchColliderWire;
#endif
};