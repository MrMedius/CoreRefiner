#pragma once
#include "Attack.h"
#include "Enemy.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"

class Ball : public Attack
{
	public:
	Ball(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 direction, Object_Type_Tag tag = attack_Ball)
		:
		Attack(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 1.0f,1.0f,1.0f });
		SetCollisionSize({ 2.0f,2.0f,2.0f });
		SetCollisionOnOff(true);
		SetMoveAccel(direction);

		// graphics init
		visualPre = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 1.0f,1.0f,1.0f });
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->LinkTechniques(rg);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(0.0f, 0.0f, 1.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		// reset parameters
		SetPosition(pos);
		boxCollider.center = transInfo.position;
		SetCollisionOnOff(true);
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;

		// reset animation
		visualPre->SetPosition(transInfo.position);
	}
	void OnEnable(void) override {};
	void Update(float dt) override
	{
		// move update
		CalculateMoveVelocity(GetMoveAccel());
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
		Deactivate();

		SetMoveAccel({0.0f, 0.0f, 0.0f});
		MoveVelocity = { 0.0f,0.0f,0.0f };
		SetCollisionOnOff(false);
		transInfo.position = other->GetPosition();
		transInfo.position.z -= 0.1f;

		if (auto* e = dynamic_cast<Enemy*>(other))
		{
			e->SetIsHurt(true);	// 攻撃された状態に遷移
			e->SetWasHurt(true);
			e->CalculateHpCurrent(-1.0f); // 体力計算

			float dx = e->GetPosition().x - transInfo.position.x;
			float dz = e->GetPosition().z - transInfo.position.z;
			float angle = atan2f(dx, dz);
			XMFLOAT3 repel{ 0.3f,0.1f,0.1f };
			other->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z); // 撃退する
		}
	}
private:
	std::unique_ptr<Ball_Shape> visualPre;
};