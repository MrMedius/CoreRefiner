#pragma once

class ObjectBase;

// Gameplay-side component contract (not UI's IUiComponent).
// Lifecycle mirrors the host ObjectBase: OnEnable / OnDisable / Update / Submit.
class IComponent
{
public:
	IComponent() = delete;
	explicit IComponent(ObjectBase* owner) noexcept
		:
		owner_(owner)
	{}
	virtual ~IComponent() = default;

	virtual void OnEnable() {}
	// Called when host Deactivate runs.
	// Stop logic / clear transient state only — do not free pooled GPU resources (Drawable etc.).
	virtual void OnDisable() {}
	virtual void Update(float dt) { (void)dt; }
	virtual void Submit() {}

	[[nodiscard]] ObjectBase* GetOwner() const noexcept { return owner_; }

private:
	ObjectBase* owner_{ nullptr };
};

// Example
//class NullComponent : public IComponent
//{
//public:
//	explicit NullComponent(ObjectBase* owner) noexcept
//		: IComponent(owner)
//	{}
//
//	void OnEnable() override {};
//	void OnDisable() override {};
//	void Update(float dt) override {};
//	void Submit() override {};
//};
