#pragma once
#include "Attack.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"
#include "VisualComponent.h"

/**
 * @brief Thin demo projectile root: world-space flight + lifetime; children are visual Orbiters.
 * @note No collider this phase — lifetime RequestDisable cascades pool recycle + ClearParent.
 */
class OrbitCore : public Attack
{
public:
	OrbitCore(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 /*direction*/, Object_Type_Tag tag = attack_OrbitCore)
		:
		Attack(tag)
	{
		SetPosition(position);
		SetSize({ 1.0f, 1.0f, 1.0f });
		lifeTime = 2.0f;

		auto shape = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 1.0f, 1.0f, 1.0f });
		shape->LinkTechniques(rg);
		AddComponent<VisualComponent>(std::move(shape), Chan::main, false, false);
	}

	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		SetPosition(pos);
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;
		ObjectBase::Update(0.0f);
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i))
			{
				child->Update(0.0f);
			}
		}
	}

	void OnEnable(void) override
	{
		lastTime = 0.0f;
		ResetMoveVelocity();
	}

	void Update(float dt) override
	{
		CalculateMoveVelocity(GetMoveAccel());
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		ObjectBase::Update(dt);

		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i); child != nullptr && child->IsActive())
			{
				child->Update(dt);
			}
		}

		lastTime += dt;
		if (lastTime >= lifeTime)
		{
			RequestDisable();
		}
	}

	void Submit(void) override
	{
		ObjectBase::Submit();
		for (std::size_t i = 0; i < GetChildCount(); ++i)
		{
			if (ObjectBase* child = GetChild(i); child != nullptr && child->IsActive())
			{
				child->Submit();
			}
		}
	}

	void OnCollide(Character* /*other*/) override
	{
		RequestDisable();
	}
};
