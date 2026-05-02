#pragma once
#include "ObjectBase.h"
#include "Character.h"

class Attack : public ObjectBase
{
public:
	Attack(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	void virtual SpawnAt(XMFLOAT3 pos, XMFLOAT3 dir) = 0;
	void OnEnable(void) override = 0;
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	virtual void OnCollide(Character* other) = 0;
public:
	void CalculateMoveVelocity(float X, float Y, float Z) { MoveVelocity.x += X; MoveVelocity.y += Y; MoveVelocity.z += Z; }
	void CalculateMoveVelocity(XMFLOAT3 offset) { CalculateMoveVelocity(offset.x, offset.y, offset.z); }
	void ResetMoveVelocity(void) { MoveVelocity = { 0.0f,0.0f,0.0f }; }
	XMFLOAT3 GetMoveVelocity(void) const { return MoveVelocity; }
	void SetMoveAccel(XMFLOAT3 accel) { MoveAccel = accel; }
	XMFLOAT3 GetMoveAccel(void) const { return MoveAccel; }
protected:
	XMFLOAT3 MoveAccel{ 0.0f,0.0f,0.0f };
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };
	float lastTime{ 0.0f };
	float lifeTime{ 0.5f };
};