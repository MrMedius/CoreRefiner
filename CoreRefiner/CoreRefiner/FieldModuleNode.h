#pragma once

#include <DirectXMath.h>

struct DeployContext;

enum class ModuleReadyState : unsigned char
{
	Ready,
	Cooling,
};

class FieldModuleNode
{
public:
	virtual ~FieldModuleNode() = default;

	[[nodiscard]] DirectX::XMFLOAT2 GetLocalPos() const noexcept { return localPos_; }
	void SetLocalPos(DirectX::XMFLOAT2 pos) noexcept { localPos_ = pos; }

	[[nodiscard]] float GetHitRadius() const noexcept { return hitRadius_; }
	void SetHitRadius(float r) noexcept { hitRadius_ = r; }

	[[nodiscard]] ModuleReadyState GetState() const noexcept { return state_; }
	[[nodiscard]] bool IsCore() const noexcept { return isCore_; }
	[[nodiscard]] bool IsReady() const noexcept { return state_ == ModuleReadyState::Ready; }

	[[nodiscard]] float GetCooldownRemaining() const noexcept { return cooldownRemaining_; }
	[[nodiscard]] float GetCooldownDuration() const noexcept { return cooldownDuration_; }
	void SetCooldownDuration(float seconds) noexcept { cooldownDuration_ = seconds; }

	void StartCooldown()
	{
		state_ = ModuleReadyState::Cooling;
		cooldownRemaining_ = cooldownDuration_;
	}

	void TickCooldown(float dt)
	{
		if (state_ != ModuleReadyState::Cooling)
		{
			return;
		}
		cooldownRemaining_ -= dt;
		if (cooldownRemaining_ <= 0.0f)
		{
			cooldownRemaining_ = 0.0f;
			state_ = ModuleReadyState::Ready;
		}
	}

	virtual void ApplyTo(DeployContext& ctx) = 0;
	[[nodiscard]] virtual const char* GetLabel() const noexcept = 0;

protected:
	FieldModuleNode() = default;

	DirectX::XMFLOAT2 localPos_{ 0.0f, 0.0f };
	float hitRadius_{ 16.0f };
	ModuleReadyState state_{ ModuleReadyState::Ready };
	float cooldownRemaining_{ 0.0f };
	float cooldownDuration_{ 3.0f };
	bool isCore_{ false };
};
