#pragma once
#include "AttackBase_Moduled.h"
#include "EnemyBase.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Drawable.h"
#include "VisualComponent.h"
#include "SphereColliderComponent.h"
#include "Framework/Helpers/Mathematics/XMath.h"
#include "AttackManager.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Bind
{
	class AttackShadeCbuf;
}

// 攻击球自己的网格。颜色编号留在 shade_ 里，只给 Ball 用。
class Ball_Shape : public Drawable
{
public:
	Ball_Shape(Graphics& gfx, DirectX::XMFLOAT3 size);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	// 清掉这一发外形上已经记下的颜色编号，并把外层透明度收回 1。
	void ClearColorIds() noexcept;
	// 按写入顺序再追加一条。colorId 为 0 是灰，1 是白，2 是黑。
	void AppendColorId(std::uint32_t colorId);
	// opacity 小于 1 时这颗球改走透明通道，并由着色器按视线把中心变透。1 时仍走原来的不透明通道。
	void SetOuterOpacity(float opacity) noexcept;
private:
	// 这一发的颜色列表。没有编号时球是灰色。
	std::shared_ptr<Bind::AttackShadeCbuf> shade_;
};

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
		{
			// 内层由球自己留着。不放进视觉组件，关着时就不会被组件循环刷新或提交。
			innerShape_ = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ kInnerScale_, kInnerScale_, kInnerScale_ });
			innerShape_->LinkTechniques(rg);
		}

		// Unit scale ⇒ radius 1 (legacy: diameter 2 box).
		pCollider_ = AddComponent<SphereColliderComponent>(1.0f, ColliderSyncMode::FollowCenter);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3(0.0f, 1.0f, 0.0f), "wireSphere");
		ApplyPresentation({ 1.0f, 1.0f, 1.0f }, true);
	}

	// 清掉外层和内层已经记下的颜色编号，并把外层透明度收回 1。内层同时关掉。
	void ClearColorIds() noexcept
	{
		if (shape_ != nullptr)
		{
			shape_->ClearColorIds();
		}
		if (innerShape_ != nullptr)
		{
			innerShape_->ClearColorIds();
		}
		SetInnerVisible(false);
	}

	// 按写入顺序再追加一条。外层和内层记同一条。colorId 为 0 是灰，1 是白，2 是黑。
	void AppendColorId(std::uint32_t colorId)
	{
		if (shape_ != nullptr)
		{
			shape_->AppendColorId(colorId);
		}
		if (innerShape_ != nullptr)
		{
			innerShape_->AppendColorId(colorId);
		}
	}

	// visible 为真时这一发才刷新并绘制内层。为假时 Ball 的更新和提交都不碰它。
	void SetInnerVisible(bool visible) noexcept
	{
		innerVisible_ = visible;
	}

	// opacity 小于 1 时这颗球按视线把中心变透。1 不透明。只改这一发的外形。
	void SetOuterOpacity(float opacity) noexcept
	{
		if (shape_ != nullptr)
		{
			shape_->SetOuterOpacity(opacity);
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
		// 这一发出生。清掉上一发留下的「已经画过」，等第一次提交后再结算命中。
		presented_ = false;
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
			// 站位由子物体模块在下面的 ArmModules 里写。生成时的玩家坐标不能当成偏移。
			childAtk->Stats().ResetMods();
			childAtk->Stats().lifetime.base = 2.0f;
			// 子弹同样先出生，再按想要的开关写入碰撞，然后才 ArmModules。
			childAtk->MarkLaunched();
			if (auto* childBall = dynamic_cast<Ball*>(childAtk))
			{
				// 子物体不走自己的 SpawnAt。这一发同样先清掉「已经画过」。
				childBall->presented_ = false;
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
	// 从对象池取出时清掉上一发的绘制标记。真正出生还要等 SpawnAt。
	void OnEnable(void) override
	{
		presented_ = false;
		// 从池里取出时先藏起内层。少阳在这一发出生时再打开。
		SetInnerVisible(false);
	}
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

		// 少阳打开后，内层跟着这一帧已经定好的位置和朝向。
		if (innerVisible_)
		{
			SyncInner_();
		}
		ObjectBase::Update(dt);
	}
	void Submit(void) override
	{
		// 这次已经交去绘制。下一帧起命中才结算。
		presented_ = true;
		// 出生那一帧可能还没进 Update。打开时先把姿态写上再提交。
		if (innerVisible_)
		{
			if (innerShape_ != nullptr)
			{
				innerShape_->Submit(Chan::main);
			}
		}
		ObjectBase::Submit();
	}
	// 给 OnCollide 看。没提交过绘制就先不结算。
	bool IsPresentedForHit_() const noexcept override
	{
		return presented_;
	}
	// 球自己的命中：停下、扣普通伤害、向上轻跳。模块的 OnHit 由 AttackBase_Moduled::OnCollide 在这之后调用。
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

			// hop 是这一次向上的速度。不加水平速度。正在上升时不超过这一次。
			constexpr float hop = 0.1f;
			other->AddHop(hop);
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

	// 把这颗球当前的世界位置、旋转和缩放写进内层外形。网格上的 0.65 不再乘一次。
	void SyncInner_()
	{
		if (innerShape_ == nullptr)
		{
			return;
		}
		innerShape_->SetPosition(GetWorldPosition());
		if (GetParent() == nullptr)
		{
			const XMFLOAT3 rot = GetRotation();
			innerShape_->SetRotationRad(rot.x, rot.y, rot.z);
			innerShape_->SetScale(GetSize());
			return;
		}

		// 挂在父物体上时，朝向和缩放从世界矩阵拆出来，公转的子弹才会跟着走。
		XMVECTOR scaleV{};
		XMVECTOR quatV{};
		XMVECTOR transV{};
		if (!XMMatrixDecompose(&scaleV, &quatV, &transV, GetWorldMatrix()))
		{
			innerShape_->SetScale(GetSize());
			return;
		}
		XMFLOAT3 scale{};
		XMStoreFloat3(&scale, scaleV);
		innerShape_->SetScale(scale);
		const XMMATRIX rotM = XMMatrixRotationQuaternion(quatV);
		XMFLOAT4X4 rot4{};
		XMStoreFloat4x4(&rot4, rotM);
		const XMFLOAT3 euler = ExtractEulerAngles(rot4);
		innerShape_->SetRotationRad(euler.x, euler.y, euler.z);
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
	// 藏在外层里的缩小外形。球自己持有，颜色编号和外层各记一份。
	std::unique_ptr<Ball_Shape> innerShape_;
	// 为真时才在 Update 里写内层姿态，并在 Submit 里提交。默认关。
	bool innerVisible_{ false };
	// 内层网格相对单位球的尺寸。碰撞仍用外层的缩放。
	static constexpr float kInnerScale_{ 0.4f };
	// 组装和出生流程想要的碰撞开关。真正开启还要等已出生。
	bool colliderWanted_{ true };
	// 这一发是否已经提交过绘制。为假时命中不结算。
	bool presented_{ false };
};
