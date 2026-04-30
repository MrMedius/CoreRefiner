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
	Item_Type_None = 0x000,
	// Character
	character_Player			   = 0x999,
	character_Enemy_Red_0		   = 0x011,
	// Environment				  
	environment_Field			   = 0x100,
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
	Object_Type_Tag GetTag(void) const		{ return Tag; }					// オブジェクト種類をチェット
	bool IsActive(void) const				{ return IsUse; }				// 使用状態をチェット
	void Activate()							{ IsUse = true;	OnEnable(); }	// 使用始まる
	void Deactivate()						{ IsUse = false; }				// 使用終わる
	XMFLOAT3 GetPosition(void) const		{ return transInfo.position; }
	XMFLOAT3 GetRotation(void) const		{ return transInfo.rotation; }
	XMFLOAT3 GetSize(void) const			{ return transInfo.scale; }
	BoxCollider	GetBoxCollider(void) const	{ return boxCollider; }
	XMFLOAT3 GetCollisionSize(void) const	{ return boxCollider.half; }
	void SetCollisionOnOff(bool OnOff)		{ OnCollision = OnOff; }
	void SetCollisionSize(XMFLOAT3 size)	{ boxCollider.half = { size.x / 2,size.y / 2 ,size.z / 2 }; }
	bool GetCollisionOnOff(void) const		{ return OnCollision; }
protected:
	void Transform(float X, float Y, float Z)	{ transInfo.position.x += X; transInfo.position.y += Y; transInfo.position.z += Z; }
	void SetPosition(XMFLOAT3 position)			{ transInfo.position = position; }
	void Rotate(float X, float Y, float Z)		{ transInfo.rotation.x += X; transInfo.rotation.y += Y; transInfo.rotation.z += Z; }
	void SetRotation(XMFLOAT3 rotate)			{ transInfo.rotation = rotate; }
	void Scale(float X, float Y, float Z)		{ transInfo.scale.x += X; transInfo.scale.y += Y; transInfo.scale.z += Z; }
	void SetSize(XMFLOAT3 size)					{ transInfo.scale = size; }
protected:
	bool IsUse{ true };
	
	Object_Type_Tag Tag{ Item_Type_None };
	
	TransInfo transInfo;

	BoxCollider boxCollider;
#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> boxColliderWire;
#endif

	bool OnCollision{ false };
};