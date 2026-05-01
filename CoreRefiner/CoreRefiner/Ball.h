#pragma once
#include "Attack.h"
#include "RenderGraph.h"
#include "Channels.h"
#include "Ball_Shape.h"

class Ball : public Attack
{
	public:
	Ball(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT3 position, XMFLOAT3 direction, Object_Type_Tag tag = attack_Ball)
		:
		Attack(tag)
	{
		// parameters init
		SetPosition(position);
		SetSize({ 1.0f,1.0f,1.0f });
		SetCollisionSize({ 1.0f,1.0f,1.0f });
		SetCollisionOnOff(true);
		SetMoveAccel(direction);

		// graphics init
		visualPre = std::make_unique<Ball_Shape>(gfx, XMFLOAT3{ 1.0f,1.0f,1.0f });
		visualPre->SetPosition(transInfo.position);
		visualPre->SetScale(transInfo.scale);
		visualPre->LinkTechniques(rg);

		// collider init
#ifdef _DEBUG
		boxColliderWire = std::make_unique<CubeWireframe>(gfx, XMFLOAT3(0.0f, 0.0f, 1.0f));
		boxColliderWire->LinkTechniques(rg);
#endif
	}
	void SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) override
	{
		// reset parameters
		SetPosition(pos);
		boxCollider.center = transInfo.position;
		SetCollisionOnOff(true);
		SetMoveAccel(dir);
		ResetMoveVelocity();
		lastTime = 0.0f;

		// reset animation
		visualPre->SetPosition(transInfo.position);
	}
	void OnEnable(void) override {};
	void Update(float dt) override
	{
		// move update
		CalculateMoveVelocity(GetMoveAccel());
		Transform(MoveVelocity.x, MoveVelocity.y, MoveVelocity.z);
		visualPre->SetPosition(transInfo.position);

		// collider update
		boxCollider.center = transInfo.position;
	}
	void Submit(void) override
	{
		visualPre->Submit(Chan::main);
#ifdef _DEBUG
		boxColliderWire->DoSubmit(transInfo.position, boxCollider.GetSize());
#endif
	}
private:
	std::unique_ptr<Ball_Shape> visualPre;
};