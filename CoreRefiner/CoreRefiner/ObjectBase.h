#pragma once
#include "Graphics.h"
#include <memory>
#include "Transformation.h"
#include "Collision.h"

#include "CubeWireframe.h"

using namespace DirectX;
using namespace Collider3D;

enum Object_Type_Tag
{
	Item_Type_None		= 0x000,
	// Attack
	attack_Ball			= 0x101,
	// Character
	character_Player	= 0x999,
	character_Enemy_T	= 0x011,
	// Environment				  
	environment_Field	= 0x100,
	// Effect
};

class ObjectBase
{
public:
	ObjectBase(Object_Type_Tag tag = Item_Type_None)
		:
		Tag(tag)
	{}
	virtual void OnEnable(void) = 0;
	virtual void Update(float dt) = 0;
	virtual void Submit(void) = 0;
	Object_Type_Tag GetTag(void) const		{ return Tag; }
	bool IsActive(void) const				{ return IsUse; }
	void Activate()							{ IsUse = true;	OnEnable(); }
	void Deactivate()						{ IsUse = false; }
	XMFLOAT3 GetPosition(void) const		{ return transform_.GetPosition(); }
	/** @brief Raw stored rotation (gameplay may keep degrees). */
	XMFLOAT3 GetRotation(void) const		{ return transform_.GetRotationRaw(); }
	XMFLOAT3 GetSize(void) const			{ return transform_.GetScale(); }
	BoxCollider	GetBoxCollider(void) const	{ return boxCollider; }
	XMFLOAT3 GetCollisionSize(void) const	{ return boxCollider.half; }
	void SetCollisionOnOff(bool OnOff)		{ OnCollision = OnOff; }
	void SetCollisionSize(XMFLOAT3 size)	{ boxCollider.half = { size.x / 2,size.y / 2 ,size.z / 2 }; }
	bool GetCollisionOnOff(void) const		{ return OnCollision; }
	/** @brief Host transform (single source of truth). */
	Transformation& GetTransform() noexcept { return transform_; }
	const Transformation& GetTransform() const noexcept { return transform_; }
protected:
	void Transform(float X, float Y, float Z)	{ transform_.Translate(X, Y, Z); }
	void SetPosition(XMFLOAT3 position)			{ transform_.SetPosition(position); }
	void Rotate(float X, float Y, float Z)		{ transform_.RotateRaw(X, Y, Z); }
	void SetRotation(XMFLOAT3 rotate)			{ transform_.SetRotationRaw(rotate); }
	void Scale(float X, float Y, float Z)		{ transform_.AddScale(X, Y, Z); }
	void SetSize(XMFLOAT3 size)					{ transform_.SetScale(size); }
protected:
	bool IsUse{ true };
	
	Object_Type_Tag Tag{ Item_Type_None };

	/** @brief Owned transform storage (single source of truth for gameplay). */
	Transformation transform_;

	BoxCollider boxCollider;
#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> boxColliderWire;
#endif

	bool OnCollision{ false };
};
