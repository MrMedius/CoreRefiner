#pragma once
#include "Environment.h"
#include "RenderGraph.h"
#include "Field_Shape.h"
#include "Channels.h"

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

		// graphics init
		visualPre = std::make_unique<Field_Shape>(gfx, size);
		visualPre->SetPosition(GetPosition());
		visualPre->LinkTechniques(rg);

		// collider init
		DirectX::XMFLOAT3 localHalf{ 0.5f, 0.5f, 0.5f };
		boxCollider = BoxCollider::BuildFromWorldMatrix(GetTransform().GetInfo().GetWorldMatrix(), localHalf);
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(1.0f, 0.0f, 0.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override {}
	void Update(float dt) override {}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
		visualPre->Submit(Chan::shadow);
#ifdef _DEBUG
		boxColliderWire->DoSubmit(GetPosition(), boxCollider.GetSize());
#endif
	}
	void OnCollide(Character* other) override {}
private:
	std::unique_ptr<Field_Shape> visualPre;
};
