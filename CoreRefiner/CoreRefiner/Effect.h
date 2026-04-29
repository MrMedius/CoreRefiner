#pragma once
#include "ObjectBase.h"
#include "Character.h"

class Effect : public ObjectBase
{
protected:
	enum EffectState
	{
		Appear,
		Play,
		Disappear,
	};
public:
	Effect(Object_Type_Tag tag)
		:
		ObjectBase(tag)
	{}
	void virtual SpawnAt(XMFLOAT3 pos, bool flip) = 0;
	void OnEnable(void) override = 0;
	void Update(float dt) override = 0;
	void Submit(void) override = 0;
	virtual void OnCollide(Character* other) = 0;
public:
	void CalculateMoveVelocity(float X, float Y, float Z)	{ MoveVelocity.x += X; MoveVelocity.y += Y; MoveVelocity.z += Z; }
	void CalculateMoveVelocity(XMFLOAT3 offset)				{ CalculateMoveVelocity(offset.x, offset.y, offset.z); }
	void ResetMoveVelocity(void)							{ MoveVelocity = { 0.0f,0.0f,0.0f }; }
	XMFLOAT3 GetMoveVelocity(void) const					{ return MoveVelocity; }											
	void SetMoveAccel(float accel)							{ MoveAccel = accel; }
	float GetMoveAccel(void) const							{ return MoveAccel; }
	void SetDoAttackType(Attack_Type_Tag type)				{ DoAttackType = type; }
	Attack_Type_Tag GetDoAttackType(void) const				{ return DoAttackType; }
protected:
	void SetEffectState(EffectState state) { effectState = state; }
protected:
	EffectState effectState{ Appear };
	float MoveAccel{ 0.0f };
	XMFLOAT3 MoveVelocity{ 0.0f,0.0f,0.0f };
	Attack_Type_Tag DoAttackType{ Attack_Type_None };
	float lastTime{ 0.0f };
	float lifeTime{ 0.5f };
};