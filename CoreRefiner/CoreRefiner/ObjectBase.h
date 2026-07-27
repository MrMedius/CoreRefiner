#pragma once
#include "Graphics.h"
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "Transformation.h"
#include "Collision.h"
#include "IComponent.h"
#include "ColliderComponent.h"

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
	void Activate()
	{
		IsUse = true;
		OnEnable();
		EnableComponents();
	}
	void Deactivate()
	{
		DisableComponents();
		IsUse = false;
	}
	XMFLOAT3 GetPosition(void) const		{ return transform_.GetPosition(); }
	/** @brief Raw stored rotation (gameplay may keep degrees). */
	XMFLOAT3 GetRotation(void) const		{ return transform_.GetRotationRaw(); }
	XMFLOAT3 GetSize(void) const			{ return transform_.GetScale(); }
	/**
	 * @brief Active box volume: prefers ColliderComponent when it registered a Box.
	 * @note Legacy host `boxCollider` remains until Phase 4.4 removal.
	 */
	BoxCollider GetBoxCollider(void) const
	{
		if (const ColliderComponent* collider = GetComponent<ColliderComponent>())
		{
			if (const BoxCollider* box = collider->TryGetBox())
			{
				return *box;
			}
		}
		return boxCollider;
	}
	XMFLOAT3 GetCollisionSize(void) const
	{
		return GetBoxCollider().half;
	}
	void SetCollisionOnOff(bool OnOff)		{ OnCollision = OnOff; }
	void SetCollisionSize(XMFLOAT3 size)
	{
		boxCollider.half = { size.x / 2, size.y / 2, size.z / 2 };
		if (ColliderComponent* collider = GetComponent<ColliderComponent>())
		{
			collider->SetCollisionSize(size);
		}
	}
	bool GetCollisionOnOff(void) const		{ return OnCollision; }
	/** @brief Host transform (single source of truth). */
	Transformation& GetTransform() noexcept { return transform_; }
	const Transformation& GetTransform() const noexcept { return transform_; }

	/**
	 * @brief Attach a component owned by this host.
	 * @tparam T Must derive from IComponent; first ctor arg is always this owner.
	 * @return Non-owning pointer to the created component.
	 */
	template <typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<IComponent, T>, "T must inherit from IComponent");
		auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* raw = component.get();
		components_.push_back(std::move(component));
		if (IsUse)
		{
			raw->OnEnable();
		}
		return raw;
	}

	/**
	 * @brief Find first attached component of type T.
	 */
	template <typename T>
	T* GetComponent() noexcept
	{
		static_assert(std::is_base_of_v<IComponent, T>, "T must inherit from IComponent");
		for (auto& c : components_)
		{
			if (T* typed = dynamic_cast<T*>(c.get()))
			{
				return typed;
			}
		}
		return nullptr;
	}
	template <typename T>
	const T* GetComponent() const noexcept
	{
		static_assert(std::is_base_of_v<IComponent, T>, "T must inherit from IComponent");
		for (const auto& c : components_)
		{
			if (const T* typed = dynamic_cast<const T*>(c.get()))
			{
				return typed;
			}
		}
		return nullptr;
	}
protected:
	void Transform(float X, float Y, float Z)	{ transform_.Translate(X, Y, Z); }
	void SetPosition(XMFLOAT3 position)			{ transform_.SetPosition(position); }
	void Rotate(float X, float Y, float Z)		{ transform_.RotateRaw(X, Y, Z); }
	void SetRotation(XMFLOAT3 rotate)			{ transform_.SetRotationRaw(rotate); }
	void Scale(float X, float Y, float Z)		{ transform_.AddScale(X, Y, Z); }
	void SetSize(XMFLOAT3 size)					{ transform_.SetScale(size); }

	/** @brief Drive attached components (not auto-called from Update yet). */
	void UpdateComponents(float dt)
	{
		for (auto& c : components_)
		{
			c->Update(dt);
		}
	}
	/** @brief Drive attached component submit (not auto-called from Submit yet). */
	void SubmitComponents()
	{
		for (auto& c : components_)
		{
			c->Submit();
		}
	}
	void EnableComponents()
	{
		for (auto& c : components_)
		{
			c->OnEnable();
		}
	}
	void DisableComponents()
	{
		for (auto& c : components_)
		{
			c->OnDisable();
		}
	}
protected:
	bool IsUse{ true };
	
	Object_Type_Tag Tag{ Item_Type_None };

	/** @brief Owned transform storage (single source of truth for gameplay). */
	Transformation transform_;

	/** @brief Owned gameplay components. */
	std::vector<std::unique_ptr<IComponent>> components_;

	/** @brief Legacy host box; used when no ColliderComponent is attached. */
	BoxCollider boxCollider;
#ifdef _DEBUG
	std::unique_ptr<CubeWireframe> boxColliderWire;
#endif

	bool OnCollision{ false };
};
