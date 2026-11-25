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
	// Character
	character_Player	= 0x999,
	character_Enemy_T	= 0x011,
	// Attack
	attack_Ball			= 0x101,
	attack_Ring_Yang	= 0x102,
	// Environment
	environment_Field	= 0x200,
	environment_Coin	= 0x201,
	// Effect
	effect_Ring_Yin			= 0x301,
	effect_ExecuteSphere	= 0x302,
};

class ObjectBase
{
public:
	ObjectBase(Object_Type_Tag tag = Item_Type_None)
		:
		Tag(tag)
	{}
	virtual ~ObjectBase();

	virtual void OnEnable(void) = 0;
	// Host-driven update: derived gameplay should call ObjectBase::Update at the end.
	// Default drives attached components (Visual/Collider sync, etc.).
	virtual void Update(float dt)
	{
		UpdateComponents(dt);
	}
	// Host-driven submit: derived should call ObjectBase::Submit at the end.
	// Default submits attached components (Drawable / debug wire).
	virtual void Submit(void)
	{
		SubmitComponents();
	}
	Object_Type_Tag GetTag(void) const		{ return Tag; }
	// True when in use and not marked for deferred disable.
	bool IsActive(void) const				{ return IsUse && !pendingDisable_; }
	// 对象池是否可拿走此实例。默认 = 非 Active。
	[[nodiscard]] virtual bool IsReusable() const noexcept { return !IsActive(); }
	void Activate();
	// Immediate disable: cascade children, detach hierarchy, OnDisable components.
	// Prefer RequestDisable() from Update/collision; use this for Reset / pool warmup / Flush.
	// AttackBase_Moduled 清模块发生在对象回收之后。
	virtual void Deactivate();

	// 标记为即将关闭，并把还挂着的变换子物体也标记关闭。帧末再真正 Deactivate。
	// 攻击球之间不是变换父子，所以这里不会关掉同一次出生的子弹。那份名单在 Deactivate 里处理。
	virtual void RequestDisable();

	// Local position (≈ Unity localPosition; ≡ world when unparented).
	XMFLOAT3 GetPosition(void) const		{ return transform_.GetPosition(); }
	// 本地旋转，单位弧度。x 是 pitch，y 是 yaw，z 是 roll。
	XMFLOAT3 GetRotation(void) const		{ return transform_.GetRotationRaw(); }
	XMFLOAT3 GetSize(void) const			{ return transform_.GetScale(); }
	Transformation& GetTransform() noexcept { return transform_; }
	const Transformation& GetTransform() const noexcept { return transform_; }

	// Local S*R*T matrix.
	[[nodiscard]] DirectX::XMMATRIX GetLocalMatrix() const noexcept;
	// World matrix: local * parent->GetWorldMatrix() (row-vector, GM31/Unity-style).
	[[nodiscard]] DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	// World-space translation extracted from GetWorldMatrix().
	[[nodiscard]] DirectX::XMFLOAT3 GetWorldPosition() const noexcept;
	// 世界空间的模型 +X。取世界矩阵第一行并归一化。
	[[nodiscard]] XMFLOAT3 GetRight() const noexcept;
	// 世界空间的模型 +Y。取世界矩阵第二行并归一化。
	[[nodiscard]] XMFLOAT3 GetUp() const noexcept;
	// 世界空间的模型 +Z。取世界矩阵第三行并归一化。
	[[nodiscard]] XMFLOAT3 GetForward() const noexcept;
	// 让本地 +X 指向 direction。没有父物体时，世界 Right 就是这个方向。方向过短则旋转归零。
	void AimRightAt(XMFLOAT3 direction);
	// 让本地 +Z 指向 direction。方向过短则旋转归零。
	void AimForwardAt(XMFLOAT3 direction);

	void SetParent(ObjectBase* parent);
	void ClearParent();
	void ResetHierarchy();
	[[nodiscard]] ObjectBase* GetParent() const noexcept { return parent_; }
	[[nodiscard]] size_t GetChildCount() const noexcept { return children_.size(); }
	[[nodiscard]] ObjectBase* GetChild(size_t index) const noexcept;


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
	template <typename T>
	bool HasComponent() const noexcept
	{
		return GetComponent<T>() != nullptr;
	}
protected:
	void Transform(float X, float Y, float Z)	{ transform_.Translate(X, Y, Z); }
	void SetPosition(XMFLOAT3 position)			{ transform_.SetPosition(position); }
	// 在本地旋转上累加，单位弧度。x 是 pitch，y 是 yaw，z 是 roll。
	void Rotate(float X, float Y, float Z)		{ transform_.RotateRaw(X, Y, Z); }
	// 设置本地旋转，单位弧度。x 是 pitch，y 是 yaw，z 是 roll。
	void SetRotation(XMFLOAT3 rotate)			{ transform_.SetRotationRaw(rotate); }
	void Scale(float X, float Y, float Z)		{ transform_.AddScale(X, Y, Z); }
	void SetSize(XMFLOAT3 size)					{ transform_.SetScale(size); }

	void UpdateComponents(float dt)
	{
		for (auto& c : components_)
		{
			c->Update(dt);
		}
	}
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
	// Queued for frame-end Deactivate; IsActive is false while set.
	bool pendingDisable_{ false };
	
	Object_Type_Tag Tag{ Item_Type_None };

	// Local TRS (≡ world when parent_ is null).
	Transformation transform_;

	// Owned gameplay components.
	std::vector<std::unique_ptr<IComponent>> components_;

private:
	// True if `ancestor` appears on the parent chain starting at `node` (inclusive).
	[[nodiscard]] static bool IsAncestorOf(const ObjectBase* ancestor, const ObjectBase* node) noexcept;
	void DetachFromParentOnly_() noexcept;
	void DetachAllChildren_() noexcept;

	ObjectBase* parent_{ nullptr };
	// Non-owning; instances remain owned by ObjectCodex.
	std::vector<ObjectBase*> children_;
};
