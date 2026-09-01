#pragma once
#include "Attack.h"
#include "Enemy.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"
#include "VisualComponent.h"
#include "SphereColliderComponent.h"
#include "XMath.h"

#include <vector>

class Ball : public Attack
{
	public:
	Ball(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 direction, Object_Type_Tag tag = attack_Ball)
		:
		Attack(tag)
	{
		SetPosition(position);
		SetMoveAccel(direction);

		{
			auto shape = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 1.0f,1.0f,1.0f });
			shape->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(shape), Chan::main, false, true);
		}

		// Unit scale ⇒ radius 1 (legacy: diameter 2 box).
		pCollider_ = AddComponent<SphereColliderComponent>(1.0f, ColliderSyncMode::FollowCenter);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3(0.0f, 1.0f, 0.0f), "wireSphere");
		ApplyPresentation({ 1.0f, 1.0f, 1.0f }, true);
	}

	/**
	 * @brief Sync host scale and sphere radius (pool-safe; unit scale ⇒ radius 1).
	 * @param scale Local visual scale (uniform intended; radius = max component).
	 * @param enableCollider Whether the sphere collider is active after apply.
	 */
	void ApplyPresentation(XMFLOAT3 scale, bool enableCollider = true)
	{
		SetSize(scale);
		if (pCollider_ != nullptr)
		{
			const float radius = (scale.x > scale.y)
				? ((scale.x > scale.z) ? scale.x : scale.z)
				: ((scale.y > scale.z) ? scale.y : scale.z);
			pCollider_->SetRadius(radius);
			pCollider_->SetEnabled(enableCollider);
			pCollider_->SyncFromOwner();
		}
	}

	/**
	 * @brief 取缩放三分量绝对值最大者（与 ApplyPresentation 半径约定一致）。
	 */
	[[nodiscard]] static float MaxScaleComponent(XMFLOAT3 scale) noexcept
	{
		return (scale.x > scale.y)
			? ((scale.x > scale.z) ? scale.x : scale.z)
			: ((scale.y > scale.z) ? scale.y : scale.z);
	}

	/**
	 * @brief Reset pose/motion; arm modules; apply size/speed from AttackStats.
	 * @note size.base 取当前组装缩放，避免把 Deployer 的 ApplyPresentation 冲回 1。
	 */
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		Stats().ResetMods();
		SetPosition(pos);
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;

		if (pCollider_ != nullptr)
		{
			pCollider_->SyncFromOwner();
		}

		const float assembled = MaxScaleComponent(GetSize());
		Stats().size.base = (assembled > 0.0f) ? assembled : 1.0f;

		ArmModules();

		SetMoveAccel((V(GetMoveAccel()) * Stats().speed.Final()).ToFloat3());
		const float sz = Stats().size.Final();
		const bool colOn = (pCollider_ != nullptr) ? pCollider_->IsEnabled() : true;
		ApplyPresentation({ sz, sz, sz }, colOn);

		std::vector<Attack*> kids;
		kids.reserve(GetChildCount());
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (auto* childAtk = dynamic_cast<Attack*>(GetChild(i));
				childAtk != nullptr && childAtk->IsActive())
			{
				kids.push_back(childAtk);
			}
		}
		for (Attack* childAtk : kids)
		{
			childAtk->Stats().ResetMods();
			if (auto* childBall = dynamic_cast<Ball*>(childAtk))
			{
				const float childAssembled = MaxScaleComponent(childBall->GetSize());
				childBall->Stats().size.base = (childAssembled > 0.0f) ? childAssembled : 1.0f;
			}
			childAtk->ArmModules();
			if (auto* childBall = dynamic_cast<Ball*>(childAtk))
			{
				childBall->SetMoveAccel((V(childBall->GetMoveAccel()) * childBall->Stats().speed.Final()).ToFloat3());
				const float childSz = childBall->Stats().size.Final();
				const bool childColOn = (childBall->pCollider_ != nullptr)
					? childBall->pCollider_->IsEnabled()
					: true;
				childBall->ApplyPresentation({ childSz, childSz, childSz }, childColOn);
			}
		}
	}
	void OnEnable(void) override {};
	void Update(float dt) override
	{
		CalculateMoveVelocity(GetMoveAccel());
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);

		DispatchOnUpdate(dt);
		UpdateChildren(dt);
		ObjectBase::Update(dt);
	}
	void Submit(void) override
	{
		ObjectBase::Submit();
		SubmitChildren();
	}
	void OnCollide(Character* other) override
	{
		DispatchOnHit(other);

		RequestDisable();

		SetMoveAccel({0.0f, 0.0f, 0.0f});
		MoveVelocity = { 0.0f,0.0f,0.0f };
		if (pCollider_ != nullptr)
		{
			pCollider_->SetEnabled(false);
		}
		auto hitPos = other->GetWorldPosition();
		hitPos.z -= 0.1f;
		SetPosition(hitPos);

		if (auto* e = dynamic_cast<Enemy*>(other))
		{
			e->SetIsHurt(true);
			e->SetWasHurt(true);
			e->CalculateHpCurrent(-Stats().damage.Final());

			const auto selfPos = GetWorldPosition();
			const Vec3 d = V(e->GetWorldPosition()) - V(selfPos);
			float angle = atan2f(d.x, d.z);
			XMFLOAT3 repel{ 0.3f,0.1f,0.1f };
			other->CalculateMoveVelocity(sinf(angle) * repel.x, repel.y, cosf(angle) * repel.z);
		}
	}
private:
	SphereColliderComponent* pCollider_{ nullptr };
};
