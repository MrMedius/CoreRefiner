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
	character_Enemy_Red_T		   = 0x010,
	character_Enemy_Red_0		   = 0x011,
	character_Enemy_Red_1		   = 0x012,
	character_Enemy_Green_T		   = 0x020,
	character_Enemy_Green_0		   = 0x021,
	character_Enemy_Green_1		   = 0x022,
	character_Enemy_Blue_T		   = 0x030,
	character_Enemy_Blue_0		   = 0x031,
	character_Enemy_Blue_1		   = 0x032,
	character_Enemy_Boss		   = 0x041,
	// Environment				  
	environment_Skybox			   = 0x100,
	environment_Block			   = 0x101,
	environment_BlockTiled		   = 0x102,
	environment_BlockInvisible	   = 0x103,
	environment_BlockFog		   = 0x104,
	environment_FireflyField	   = 0x105,
	environment_SkyGridField	   = 0x106,
	environment_BlockTiledTutorial = 0x107,
	// Effect
	effect_Player_Remote_Attack_1  = 0x200,
	effect_Player_Remote_Attack_2  = 0x201,
	effect_Player_Remote_Attack_3  = 0x202,
	effect_Player_Hit			   = 0x212,
	effect_Player_EnergyAbsorb	   = 0x213,
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