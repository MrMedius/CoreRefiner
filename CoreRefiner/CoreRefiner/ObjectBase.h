#pragma once
#include "Graphics.h"
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "Transformation.h"
#include "IComponent.h"
#include "DeferredDisableQueue.h"

using namespace DirectX;

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
	/**
	 * @brief Host-driven update: derived gameplay should call ObjectBase::Update at the end.
	 * @note Default drives attached components (Visual/Collider sync, etc.).
	 */
	virtual void Update(float dt)
	{
		UpdateComponents(dt);
	}
	/**
	 * @brief Host-driven submit: derived should call ObjectBase::Submit at the end.
	 * @note Default submits attached components (Drawable / debug wire).
	 */
	virtual void Submit(void)
	{
		SubmitComponents();
	}
	Object_Type_Tag GetTag(void) const		{ return Tag; }
	/**
	 * @brief True when in use and not marked for deferred disable.
	 */
	bool IsActive(void) const				{ return IsUse && !pendingDisable_; }
	void Activate()
	{
		DeferredDisableQueue::Get().Remove(this);
		pendingDisable_ = false;
		IsUse = true;
		OnEnable();
		EnableComponents();
	}
	/**
	 * @brief Immediate disable: OnDisable components, clear active flags, drop from defer queue.
	 * @note Prefer RequestDisable() from Update/collision; use this for Reset / pool warmup / Flush.
	 */
	void Deactivate()
	{
		DeferredDisableQueue::Get().Remove(this);
		if (!IsUse && !pendingDisable_)
		{
			return;
		}
		DisableComponents();
		IsUse = false;
		pendingDisable_ = false;
	}
	/**
	 * @brief Mark inactive for queries and queue Deactivate at frame end (SetDestroy equivalent).
	 */
	void RequestDisable()
	{
		if (!IsUse || pendingDisable_)
		{
			return;
		}
		pendingDisable_ = true;
		DeferredDisableQueue::Get().Enqueue(this);
	}
	XMFLOAT3 GetPosition(void) const		{ return transform_.GetPosition(); }
	/** @brief Raw stored rotation (gameplay may keep degrees). */
	XMFLOAT3 GetRotation(void) const		{ return transform_.GetRotationRaw(); }
	XMFLOAT3 GetSize(void) const			{ return transform_.GetScale(); }
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
		if (IsUse && !pendingDisable_)
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
	/**
	 * @brief Whether a component of type T is attached.
	 */
	template <typename T>
	bool HasComponent() const noexcept
	{
		return GetComponent<T>() != nullptr;
	}
protected:
	void Transform(float X, float Y, float Z)	{ transform_.Translate(X, Y, Z); }
	void SetPosition(XMFLOAT3 position)			{ transform_.SetPosition(position); }
	void Rotate(float X, float Y, float Z)		{ transform_.RotateRaw(X, Y, Z); }
	void SetRotation(XMFLOAT3 rotate)			{ transform_.SetRotationRaw(rotate); }
	void Scale(float X, float Y, float Z)		{ transform_.AddScale(X, Y, Z); }
	void SetSize(XMFLOAT3 size)					{ transform_.SetScale(size); }

	/**
	 * @brief Drive attached component Update (used by ObjectBase::Update).
	 * @note Prefer ending derived Update with ObjectBase::Update(dt) rather than calling this directly.
	 */
	void UpdateComponents(float dt)
	{
		for (auto& c : components_)
		{
			c->Update(dt);
		}
	}
	/**
	 * @brief Drive attached component Submit (used by ObjectBase::Submit).
	 * @note Prefer ending derived Submit with ObjectBase::Submit() rather than calling this directly.
	 */
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
	/** @brief Queued for frame-end Deactivate; IsActive is false while set. */
	bool pendingDisable_{ false };
	
	Object_Type_Tag Tag{ Item_Type_None };

	/** @brief Owned transform storage (single source of truth for gameplay). */
	Transformation transform_;

	/** @brief Owned gameplay components. */
	std::vector<std::unique_ptr<IComponent>> components_;
};
