#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "Field_Shape.h"
#include "Channels.h"
#include "VisualComponent.h"
#include "BoxColliderComponent.h"

class Field : public Environment
{
public:
	Field(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 size, bool onCollision, Object_Type_Tag tag = environment_Field)
		:
		Environment(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize(size);

		// graphics init — VisualComponent owns the Drawable (mesh baked to size; syncScale=false)
		{
			auto shape = std::make_unique<Field_Shape>(gfx, size);
			shape->LinkTechniques(rg);
			AddComponent<VisualComponent>(std::move(shape), Chan::main | Chan::shadow, false, false);
		}

		// BoxColliderComponent + FromWorldMatrix (do not SetCollisionSize — it would overwrite matrix half)
		DirectX::XMFLOAT3 localHalf{ 0.5f, 0.5f, 0.5f };
		pCollider_ = AddComponent<BoxColliderComponent>(
			ColliderSyncMode::FromWorldMatrix,
			localHalf);
		pCollider_->SetEnabled(onCollision);
		pCollider_->LinkDebugWire(gfx, rg, XMFLOAT3(1.0f, 0.0f, 0.0f), "wireFieldBox");
	}
	void OnEnable(void) override {}
	void Update(float dt) override
	{
		ObjectBase::Update(dt);
	}
	void Submit(void) override
	{
		ObjectBase::Submit();
	}
	void OnCollide(Character* other) override {}
private:
	BoxColliderComponent* pCollider_{ nullptr };
};
