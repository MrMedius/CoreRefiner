#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "ColliderComponent.h"
#include "ObjectCodex.h"
#include "Ball.h"

/**
 * @brief Debug-only static Capsule probe for Sphere/Box overlap testing.
 * @note No mesh visual — cyan CapsuleWireframe only. Position near origin for easy reach.
 */
class CapsuleProbe : public Environment
{
public:
	CapsuleProbe(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		XMFLOAT3 position = { 5.0f, 3.0f, 5.0f },
		Object_Type_Tag tag = environment_CapsuleProbe)
		:
		Environment(tag)
	{
		SetPosition(position);
		SetSize({ 2.0f, 4.0f, 2.0f });

		pCollider_ = AddComponent<ColliderComponent>(
			Collider3D::CollideType::Capsule,
			ColliderSyncMode::FollowCenter);
		// radius 1, total height 4 → segment |B-A| = 2
		pCollider_->SetCapsule(1.0f, 4.0f);
		pCollider_->SetEnabled(true);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3{ 0.0f, 1.0f, 1.0f }, "wireCapsuleProbe");
	}

	void OnEnable(void) override {}

	void Update(float dt) override
	{
		ObjectBase::Update(dt);

		// Prove Sphere ∩ Capsule: disable active Balls that overlap this probe.
		auto* selfCol = pCollider_;
		if (selfCol == nullptr || !selfCol->IsEnabled())
		{
			return;
		}
		auto balls = ObjectCodex::FindActiveObjectsByTag<Ball>(attack_Ball);
		for (Ball* ball : balls)
		{
			if (ball == nullptr)
			{
				continue;
			}
			auto* ballCol = ball->GetComponent<ColliderComponent>();
			if (ballCol == nullptr || !ballCol->IsEnabled())
			{
				continue;
			}
			if (CollisionSystem::IsOverlap(selfCol->GetVolume(), ballCol->GetVolume()))
			{
				ball->RequestDisable();
				ballCol->SetEnabled(false);
			}
		}
	}

	void Submit(void) override
	{
		ObjectBase::Submit();
	}

	void OnCollide(Character* other) override
	{
		(void)other;
	}

private:
	ColliderComponent* pCollider_{ nullptr };
};
