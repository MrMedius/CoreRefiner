#pragma once

#include "Canvas2D.h"
#include "Canvas2DSpriteUV.h"
#include "ModuleNodeLabel.h"
#include "Colors.h"

#include <cstdint>
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

namespace ModuleNodeKindFill
{
	inline constexpr Color kFill[] = {
		Color(255u, 255u,   0u, 255u), // Core
		Color(255u, 180u,  60u, 255u), // Spawn
		Color(120u, 200u, 255u, 255u), // Attribute
		Color(140u, 220u, 130u, 255u), // Rule
		Color(255u, 150u,  70u, 255u), // Passive
		Color(200u, 160u, 255u, 255u), // Other
	};

	static_assert(sizeof(kFill) / sizeof(kFill[0]) == ModuleNodeKindCount(), "ModuleNodeKindFill::kFill size must match ModuleNodeKindCount");
}

class IModuleNode
{
public:
	static constexpr unsigned kVisualSize = 16u;

	virtual ~IModuleNode() = default;

	[[nodiscard]] std::uint32_t GetInstanceId() const noexcept { return instanceId_; }

	[[nodiscard]] DirectX::XMFLOAT2 GetLocalPos() const noexcept { return localPos_; }
	void SetLocalPos(DirectX::XMFLOAT2 pos) noexcept { localPos_ = pos; }

	[[nodiscard]] float GetHitRadius() const noexcept { return hitRadius_; }
	void SetHitRadius(float r) noexcept { hitRadius_ = r; }

	/** @brief 槽位残影 / 静置图标用半径；未覆盖时与命中半径相同。 */
	[[nodiscard]] float GetVisualRadius() const noexcept
	{
		return (visualRadiusOverride_ > 0.0f) ? visualRadiusOverride_ : hitRadius_;
	}

	/** @brief 当前跟随鼠标的图标半径；拖起时一律用 hitRadius_。 */
	[[nodiscard]] float GetIconRadius() const noexcept
	{
		return layoutGhostActive_ ? hitRadius_ : GetVisualRadius();
	}

	/** @brief 覆盖绘制半径；传入 <= 0 等效于清除覆盖。 */
	void SetVisualRadiusOverride(float r) noexcept
	{
		visualRadiusOverride_ = r;
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

	/** @brief 取消绘制半径覆盖，恢复为命中半径。 */
	void ClearVisualRadiusOverride() noexcept
	{
		visualRadiusOverride_ = 0.0f;
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

	[[nodiscard]] ModuleReadyState GetState() const noexcept { return state_; }
	[[nodiscard]] bool IsCore() const noexcept { return GetKind() == ModuleNodeKind::Core; }
	[[nodiscard]] bool IsReady() const noexcept { return state_ == ModuleReadyState::Ready; }

	[[nodiscard]] float GetCooldownRemaining() const noexcept { return cooldownRemaining_; }
	[[nodiscard]] float GetCooldownDuration() const noexcept { return cooldownDuration_; }
	void SetCooldownDuration(float seconds) noexcept { cooldownDuration_ = seconds; }

	[[nodiscard]] float GetScanMaxRadius() const noexcept { return scanMaxRadius_; }
	void SetScanMaxRadius(float radius) noexcept { scanMaxRadius_ = radius; }

	[[nodiscard]] float GetScanExpandSpeed() const noexcept { return scanExpandSpeed_; }
	void SetScanExpandSpeed(float speed) noexcept { scanExpandSpeed_ = speed; }

	void StartCooldown()
	{
		state_ = ModuleReadyState::Cooling;
		cooldownRemaining_ = cooldownDuration_;
	}

	void ResetCooldown() noexcept
	{
		state_ = ModuleReadyState::Ready;
		cooldownRemaining_ = 0.0f;
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

	virtual void InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 zoneOrigin);

	void SetZoneOrigin(DirectX::XMFLOAT3 zoneOrigin) noexcept;

	void SetZoneVisualScale(float scale) noexcept;

	void SyncVisual();

	void SubmitVisual();

	void BeginLayoutGhost(DirectX::XMFLOAT2 at) noexcept;

	void EndLayoutGhost() noexcept;

	[[nodiscard]] bool IsLayoutGhostActive() const noexcept { return layoutGhostActive_; }

	virtual void ApplyTo(DeployContext& ctx) = 0;
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept = 0;
	/** @brief 大类；默认 Other。未覆写的新节点会走 Other 色。 */
	[[nodiscard]] virtual ModuleNodeKind GetKind() const noexcept
	{
		return ModuleNodeKind::Other;
	}

protected:
	IModuleNode() noexcept
		:
		instanceId_(++s_nextInstanceId_)
	{}

	[[nodiscard]] virtual Color GetReadyFillColor() const noexcept;

	void ApplyVisualTransform_();
	void SyncMaskUV_();
	[[nodiscard]] float GetRemainRatio_() const noexcept;

	std::uint32_t instanceId_{ 0 };
	DirectX::XMFLOAT2 localPos_{ 0.0f, 0.0f };
	float hitRadius_{ 16.0f };
	/** @brief <= 0 表示不覆盖，绘制走 hitRadius_。 */
	float visualRadiusOverride_{ 0.0f };
	ModuleReadyState state_{ ModuleReadyState::Ready };
	float cooldownRemaining_{ 0.0f };
	float cooldownDuration_{ 3.0f };
	float scanMaxRadius_{ 140.0f };
	float scanExpandSpeed_{ 100.0f };

	std::unique_ptr<Canvas2D> icon_;
	std::unique_ptr<Canvas2DSpriteUV> mask_;
	DirectX::XMFLOAT3 zoneOrigin_{ 0.0f, 0.0f, 0.0f };
	float zoneVisualScale_{ 1.0f };
	bool visualReady_{ false };
	bool layoutGhostActive_{ false };
	DirectX::XMFLOAT2 layoutGhostLocalPos_{ 0.0f, 0.0f };

private:
	static std::uint32_t s_nextInstanceId_;
};
