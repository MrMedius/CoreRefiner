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
		SetMoveAccel(direction);

		// graphics init — VisualComponent owns the Drawable
		{
			auto shape = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 1.0f,1.0f,1.0f });
			shape->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(shape), Chan::main, false, true);
		}

		pCollider_ = AddComponent<ColliderComponent>(
			Collider3D::CollideType::Box,
			ColliderSyncMode::FollowCenter);
		pCollider_->SetCollisionSize({ 2.0f,2.0f,2.0f });
		pCollider_->SetEnabled(true);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3(0.0f, 0.0f, 1.0f));
	}
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		// reset parameters
		SetPosition(pos);
		if (pCollider_ != nullptr)
		{
			pCollider_->SetEnabled(true);
		}
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;

		// sync pooled visual + collider without re-adding components
		ObjectBase::Update(0.0f);
	}
	void OnEnable(void) override {};
	void Update(float dt) override
	{
		CalculateMoveVelocity(GetMoveAccel());
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		ObjectBase::Update(dt);
	}
	void Submit(void) override
	{
		ObjectBase::Submit();
	}
	void OnCollide(Character* other) override
	{
		RequestDisable();

		SetMoveAccel({0.0f, 0.0f, 0.0f});
		MoveVelocity = { 0.0f,0.0f,0.0f };
		if (pCollider_ != nullptr)
		{
			pCollider_->SetEnabled(false);
		}
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
	ColliderComponent* pCollider_{ nullptr };
};
