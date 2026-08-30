#pragma once
#include "IProjectileModule.h"
#include "Attack.h"
#include "XMath.h"

#include <cmath>

// 绕父物体本地 XZ 公转；角速度 = owner.Stats().speed.Final()。
// 无父物体时不改位置（父弹销毁后子弹按自身速度继续飞）。
class Rule_Orbit_Module : public IProjectileModule
{
public:
	Rule_Orbit_Module(Attack* owner, float radius, float phase0) noexcept
		:
		IProjectileModule(owner),
		radius_(radius),
		phase0_(phase0),
		angle_(phase0)
	{}

	[[nodiscard]] bool HasModuleNodeLabel() const noexcept override { return true; }
	[[nodiscard]] ModuleNodeLabel GetModuleNodeLabel() const noexcept override
	{
		return ModuleNodeLabel::Rule_Orbit;
	}

	void OnSpawn() override
	{
		angle_ = phase0_;
		ApplyLocalPose_();
	}

	void OnUpdate(float dt) override
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || owner->GetParent() == nullptr)
		{
			return;
		}
		const float omega = 1.0f + owner->Stats().speed.Final();
		angle_ += omega * dt;
		ApplyLocalPose_();
	}

	void OnRecycle() override
	{
		angle_ = phase0_;
	}

	void SetRadius(float radius) noexcept
	{
		radius_ = radius;
		ApplyLocalPose_();
	}

	void SetPhase0(float phase0) noexcept
	{
		phase0_ = phase0;
		angle_ = phase0;
		ApplyLocalPose_();
	}

private:
	void ApplyLocalPose_()
	{
		Attack* owner = GetOwner();
		if (owner == nullptr || owner->GetParent() == nullptr)
		{
			return;
		}
		owner->SetLocalPosition(Vec3{
			radius_ * std::cos(angle_),
			0.0f,
			radius_ * std::sin(angle_)
		});
	}

	float radius_{ 2.0f };
	float phase0_{ 0.0f };
	float angle_{ 0.0f };
};
