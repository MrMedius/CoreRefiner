#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "Field_Shape.h"
#include "Channels.h"
#include "VisualComponent.h"
#include "ColliderComponent.h"

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
		SetCollisionSize(size);
		SetCollisionOnOff(onCollision);

		// graphics init (mesh already baked to `size`; do not syncScale)
		visualPre = std::make_unique<Field_Shape>(gfx, size);
		visualPre->LinkTechniques(rg);
		AddComponent<VisualComponent>(visualPre.get(), Chan::main | Chan::shadow, false, false);

		// register Box rebuilt from world matrix each sync
		DirectX::XMFLOAT3 localHalf{ 0.5f, 0.5f, 0.5f };
		pCollider_ = AddComponent<ColliderComponent>(
			Collider3D::CollideType::Box,
			ColliderSyncMode::FromWorldMatrix,
			localHalf);
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(1.0f, 0.0f, 0.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override {}
	void Update(float dt) override
	{
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
	void OnCollide(Character* other) override {}
private:
	std::unique_ptr<Field_Shape> visualPre;
	ColliderComponent* pCollider_{ nullptr };
};
