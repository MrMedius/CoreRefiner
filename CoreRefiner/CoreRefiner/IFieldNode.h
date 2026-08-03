#pragma once

#include "Canvas2D.h"
#include "Canvas2DSpriteUV.h"
#include "Colors.h"

#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

struct DeployContext;

enum class ModuleReadyState : unsigned char
{
	Ready,
	Cooling,
};

class IFieldNode
{
public:
	static constexpr unsigned kVisualSize = 16u;

	virtual ~IFieldNode() = default;

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

	virtual void InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 fieldOrigin);

	void SyncVisual();

	void SubmitVisual();

	virtual void ApplyTo(DeployContext& ctx) = 0;
	[[nodiscard]] virtual const char* GetLabel() const noexcept = 0;

protected:
	IFieldNode() = default;

	[[nodiscard]] virtual Color GetReadyFillColor() const noexcept;

	void ApplyVisualTransform_();
	/** @brief 仅核心节点在 Ellipse 底色上叠加外环；非核心不改像素。 */
	void PaintIcon_();
	void SyncMaskUV_();
	[[nodiscard]] float GetRemainRatio_() const noexcept;

	DirectX::XMFLOAT2 localPos_{ 0.0f, 0.0f };
	float hitRadius_{ 16.0f };
	ModuleReadyState state_{ ModuleReadyState::Ready };
	float cooldownRemaining_{ 0.0f };
	float cooldownDuration_{ 3.0f };
	bool isCore_{ false };

	std::unique_ptr<Canvas2D> icon_;
	std::unique_ptr<Canvas2DSpriteUV> mask_;
	DirectX::XMFLOAT3 fieldOrigin_{ 0.0f, 0.0f, 0.0f };
	bool visualReady_{ false };
};
