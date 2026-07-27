#pragma once
#include "Attack.h"
#include "Enemy.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"
#include "VisualComponent.h"
#include "ColliderComponent.h"

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
		visualPre->LinkTechniques(rg);
		AddComponent<VisualComponent>(visualPre.get(), Chan::main, false, true);

		pCollider_ = AddComponent<ColliderComponent>(
			Collider3D::CollideType::Box,
			ColliderSyncMode::FollowCenter);

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
		SetCollisionOnOff(true);
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;

		// sync pooled visual + collider without re-adding components
		UpdateComponents(0.0f);
	}
	void OnEnable(void) override {};
	void Update(float dt) override
	{
		CalculateMoveVelocity(GetMoveAccel());
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		UpdateComponents(dt);
	}
	void Submit(void) override
	{
		SubmitComponents();
#ifdef _DEBUG
		auto box = GetBoxCollider();
		boxColliderWire->DoSubmit(GetPosition(), box.GetSize());
#endif
	}
	void OnCollide(Character* other) override
	{
		Deactivate();

		SetMoveAccel({0.0f, 0.0f, 0.0f});
		MoveVelocity = { 0.0f,0.0f,0.0f };
		SetCollisionOnOff(false);
		auto hitPos = other->GetPosition();
		hitPos.z -= 0.1f;
		SetPosition(hitPos);

		if (auto* e = dynamic_cast<Enemy*>(other))
		{
			e->SetIsHurt(true);
			e->SetWasHurt(true);
			e->CalculateHpCurrent(-1.0f);

			const auto selfPos = GetPosition();
			float dx = e->GetPosition().x - selfPos.x;
			float dz = e->GetPosition().z - selfPos.z;
			float angle = atan2f(dx, dz);
			XMFLOAT3 repel{ 0.3f,0.1f,0.1f };
			other->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z);
		}
	}
private:
	std::unique_ptr<Ball_Shape> visualPre;
	ColliderComponent* pCollider_{ nullptr };
};
