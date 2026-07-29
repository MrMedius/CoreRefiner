#pragma once
#include "ObjectBase.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"
#include "VisualComponent.h"

#include <cmath>
#include <cstddef>

/**
 * @brief Visual-only orbit child: drives local XZ polar pose; no collider.
 * @note World follow comes from parent hierarchy (GetWorldMatrix).
 */
class Orbiter : public ObjectBase
{
public:
	/**
	 * @brief Construct pooled visual orbiter (Ball_Shape, smaller than OrbitCore).
	 */
	Orbiter(Graphics& gfx, Rgph::RenderGraph& rg, Object_Type_Tag tag = attack_Orbiter)
		:
		ObjectBase(tag)
	{
		SetPosition({ 0.0f, 0.0f, 0.0f });
		SetSize({ 1.0f, 1.0f, 1.0f });
		auto shape = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 0.35f, 0.35f, 0.35f });
		shape->LinkTechniques(rg);
		AddComponent<VisualComponent>(std::move(shape), Chan::main, false, false);
	}

	/**
	 * @brief Configure evenly spaced orbit slot (local space).
	 * @param index Slot index in [0, count).
	 * @param count Total siblings around the parent.
	 * @param radius Local XZ orbit radius.
	 * @param angularSpeed Radians per second about parent Y.
	 */
	void ConfigureOrbit(std::size_t index, std::size_t count, float radius, float angularSpeed)
	{
		radius_ = radius;
		angularSpeed_ = angularSpeed;
		const float step = (count > 0)
			? (DirectX::XM_2PI / static_cast<float>(count))
			: 0.0f;
		phase0_ = step * static_cast<float>(index);
		angle_ = phase0_;
		ApplyLocalPose_();
	}

	void OnEnable(void) override
	{
		angle_ = phase0_;
		ApplyLocalPose_();
	}

	void Update(float dt) override
	{
		angle_ += angularSpeed_ * dt;
		ApplyLocalPose_();
		ObjectBase::Update(dt);
	}

	void Submit(void) override
	{
		ObjectBase::Submit();
	}

private:
	/** @brief Write local XZ from current angle_/radius_. */
	void ApplyLocalPose_()
	{
		SetPosition({
			radius_ * std::cos(angle_),
			0.0f,
			radius_ * std::sin(angle_)
		});
	}

	float radius_{ 2.0f };
	float angularSpeed_{ 3.5f };
	float phase0_{ 0.0f };
	float angle_{ 0.0f };
};
