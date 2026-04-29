#pragma once
#include "Environment.h"
#include "Character.h"

class BlockInvisible : public Environment
{
public:
	BlockInvisible(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 size, bool onCollision, Object_Type_Tag tag = environment_BlockInvisible)
		:
		Environment(tag),
		Rg(rg)
	{
		// parameters init
		SetPosition(position);
		SetSize(size);
		SetCollisionSize(size);
		SetCollisionOnOff(onCollision);

		// collider init
		DirectX::XMFLOAT3 localHalf{ 0.5f, 0.5f, 0.5f };
		boxCollider = BoxCollider::BuildFromWorldMatrix(transInfo.GetWorldMatrix(), localHalf);
		BlockXorZ = size.x < size.z;
		BlockPorM = BlockXorZ ? position.x < 0.0f : position.z < 0.0f;
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(1.0f, 0.0f, 0.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void OnEnable(void) override {}
	void Update(float dt) override {}
	void Submit(void) override 
	{
#ifdef _DEBUG
		boxColliderWire->DoSubmit(GetPosition(), GetSize());
#endif
	}
	void OnCollide(Character* character) override 
	{
		float pushPower = (BlockPorM ? 1.0f : -1.0f) * 0.5f;

		XMFLOAT3 v = character->GetMoveVelocity();
		XMFLOAT3 velocity = BlockXorZ ?
			XMFLOAT3{ -v.x + pushPower, 0.0f, 0.0f } :
			XMFLOAT3{ 0.0f, 0.0f, -v.z + pushPower };

		character->CalculateMoveVelocity(velocity);

		Rg.Interaction();
	}
private:
	bool BlockXorZ{ false };
	bool BlockPorM{ false };
	Rgph::RenderGraph& Rg;
};