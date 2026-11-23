#pragma once
#include "AttackBase_Moduled.h"
#include "EnemyBase.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"
#include "VisualComponent.h"
#include "SphereColliderComponent.h"
#include "XMath.h"
#include "AttackManager.h"

#include <cstdint>
#include <vector>

class Ball : public AttackBase_Moduled
{
	public:
	Ball(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 direction, Object_Type_Tag tag = attack_Ball)
		:
		AttackBase_Moduled(tag)
	{
		SetPosition(position);
		SetMoveAccel(direction);

		{
			auto shape = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 1.0f,1.0f,1.0f });
			// shape_ 留住这一发的外形。所有权交给视觉组件后，这个指针仍然指向它。
			shape_ = shape.get();
			shape->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(shape), Chan::main, false, true);
		}

		// Unit scale ⇒ radius 1 (legacy: diameter 2 box).
		pCollider_ = AddComponent<SphereColliderComponent>(1.0f, ColliderSyncMode::FollowCenter);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3(0.0f, 1.0f, 0.0f), "wireSphere");
		ApplyPresentation({ 1.0f, 1.0f, 1.0f }, true);
	}

	// 清掉这一发已经记下的颜色编号。下次出生由模块重新写入。
	void ClearColorIds() noexcept
	{
		if (shape_ != nullptr)
		{
			shape_->ClearColorIds();
		}
	}

	// 按写入顺序再追加一条。colorId 为 0 是灰，1 是白，2 是黑。
	void AppendColorId(std::uint32_t colorId)
	{
		if (shape_ != nullptr)
		{
			shape_->AppendColorId(colorId);
		}
	}

	// 记下缩放，以及组装想要的碰撞开关。
	// scale 视觉缩放（半径取三分量最大者）。
	// enableCollider 记到 colliderWanted_。未出生时碰撞仍保持关闭。
	void ApplyPresentation(XMFLOAT3 scale, bool enableCollider = true)
	{
		SetSize(scale);
		colliderWanted_ = enableCollider;
		if (pCollider_ != nullptr)
		{
			pCollider_->SetRadius(MaxScaleComponent(scale));
			pCollider_->SetEnabled(colliderWanted_ && IsLaunched());
			pCollider_->SyncFromOwner();
		}
	}

	// 取缩放三分量绝对值最大者（与 ApplyPresentation 半径约定一致）。
	[[nodiscard]] static float MaxScaleComponent(XMFLOAT3 scale) noexcept
	{
		return (scale.x > scale.y)
			? ((scale.x > scale.z) ? scale.x : scale.z)
			: ((scale.y > scale.z) ? scale.y : scale.z);
	}

	// Reset pose/motion; arm modules; apply size/speed from AttackStats.
	// size.base 取当前组装缩放，避免把 Deployer 的 ApplyPresentation 冲回 1。
	// 子弹从父加速度只取方向，再写成 kAimSpeed × 自身 speed.Final()，避免连乘。
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		Stats().ResetMods();
		SetPosition(pos);
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;

		// 先记为已出生，再按组装时想要的开关写入碰撞。之后 ArmModules 里的公转可以再关掉。
		MarkLaunched();
		OpenColliderForLaunch_();

		const float assembled = MaxScaleComponent(GetSize());
		Stats().size.base = (assembled > 0.0f) ? assembled : 1.0f;

		Stats().lifetime.base = 2.0f;
		// 出生前清掉上一发留下的颜色。模块在 ArmModules 里按顺序重新写入。
		ClearColorIds();
		ArmModules();
		SetLifeTime(Stats().lifetime.Final());

		SetMoveAccel((V(GetMoveAccel()) * Stats().speed.Final()).ToFloat3());
		// 本地 +Z 对准这一发的飞行方向。
		AimForwardAt(GetMoveAccel());
		const float sz = Stats().size.Final();
		// 只刷新缩放和半径。碰撞开关已经由出生和模块决定。
		ApplyScaleKeepCollider_({ sz, sz, sz });

		const std::vector<AttackBase_Moduled*> kids = GetBornWith();
		for (AttackBase_Moduled* childAtk : kids)
		{
			if (childAtk == nullptr || !childAtk->IsActive())
			{
				continue;
			}
			// 组装时记下的是相对主体的偏移。加在主体跑完模块后的位置上。
			// 返回会把主体挪走，不能再用传入的发射点。
			const XMFLOAT3 corePos = GetPosition();
			const XMFLOAT3 offset = childAtk->GetPosition();
			childAtk->SetLocalPosition({
				corePos.x + offset.x,
				corePos.y + offset.y,
				corePos.z + offset.z
			});
			childAtk->Stats().ResetMods();
			childAtk->Stats().lifetime.base = 2.0f;
			// 子弹同样先出生，再按想要的开关写入碰撞，然后才 ArmModules。
			childAtk->MarkLaunched();
			if (auto* childBall = dynamic_cast<Ball*>(childAtk))
			{
				const float childAssembled = MaxScaleComponent(childBall->GetSize());
				childBall->Stats().size.base = (childAssembled > 0.0f) ? childAssembled : 1.0f;
				childBall->OpenColliderForLaunch_();
				// 子弹同样先清掉上一发的颜色，再由自己的模块写入。
				childBall->ClearColorIds();
			}
			childAtk->ArmModules();
			childAtk->SetLifeTime(childAtk->Stats().lifetime.Final());
			if (auto* childBall = dynamic_cast<Ball*>(childAtk))
			{
				XMFLOAT3 launch = childBall->GetMoveAccel();
				if (launch.x == 0.0f && launch.y == 0.0f && launch.z == 0.0f)
				{
					launch = childBall->GetMoveVelocity();
				}
				if (NormalizeXZ(launch))
				{
					launch = (V(launch) * AttackManager::kAimSpeed).ToFloat3();
					childBall->SetMoveAccel(
						(V(launch) * childBall->Stats().speed.Final()).ToFloat3());
				}
				else
				{
					childBall->SetMoveAccel({ 0.0f, 0.0f, 0.0f });
				}
				const float childSz = childBall->Stats().size.Final();
				childBall->ApplyScaleKeepCollider_({ childSz, childSz, childSz });
			}
		}
	}
	void OnEnable(void) override {};
	void Update(float dt) override
	{
		// start 是本帧移动前的位置。公转会在 DispatchOnUpdate 里改位置。
		const XMFLOAT3 start = GetPosition();
		CalculateMoveVelocity(GetMoveAccel());
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);

		DispatchOnUpdate(dt);
		if (IsActive() && TickLifeTimer(dt))
		{
			RequestDisable();
		}

		// step 是本帧实际走的位移。公转时它沿着切线，直线飞行时它就是本帧速度。
		const XMFLOAT3 end = GetPosition();
		const XMFLOAT3 step{
			end.x - start.x,
			end.y - start.y,
			end.z - start.z
		};
		const float stepSq = step.x * step.x + step.y * step.y + step.z * step.z;
		if (stepSq >= 1e-8f)
		{
			AimForwardAt(step);
		}
		else
		{
			// 位移几乎为 0 时再看速度。速度也接近 0 就保持上一帧朝向。
			const XMFLOAT3 velocity = GetMoveVelocity();
			const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;
			if (speedSq >= 1e-8f)
			{
				AimForwardAt(velocity);
			}
		}

		ObjectBase::Update(dt);
	}
	void Submit(void) override
	{
		ObjectBase::Submit();
	}
	// 球自己的命中：停下、扣普通伤害、击退。模块的 OnHit 由 AttackBase_Moduled::OnCollide 在这之后调用。
	void ApplyOwnHit(CharacterBase* other) override
	{
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

		if (auto* e = dynamic_cast<EnemyBase*>(other))
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
	// 按 colliderWanted_ 写入碰撞开关。须在 MarkLaunched 之后、ArmModules 之前调用。
	void OpenColliderForLaunch_()
	{
		if (pCollider_ == nullptr)
		{
			return;
		}
		pCollider_->SetEnabled(colliderWanted_ && IsLaunched());
		pCollider_->SyncFromOwner();
	}

	// 只改缩放和碰撞半径，不动碰撞开关。出生后模块可能已经改过开关。
	void ApplyScaleKeepCollider_(XMFLOAT3 scale)
	{
		SetSize(scale);
		if (pCollider_ == nullptr)
		{
			return;
		}
		pCollider_->SetRadius(MaxScaleComponent(scale));
		pCollider_->SyncFromOwner();
	}

	SphereColliderComponent* pCollider_{ nullptr };
	// 这一发的外形。负责把颜色编号写进它的着色缓冲。
	Ball_Shape* shape_{ nullptr };
	// 组装和出生流程想要的碰撞开关。真正开启还要等已出生。
	bool colliderWanted_{ true };
};
