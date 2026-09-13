#pragma once

#include "Canvas2D.h"
#include "Canvas2DSpriteUV.h"
#include "ModuleNodeLabel.h"
#include "Colors.h"

#include <cstddef>
#include <cstdint>
#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

struct DeployContext;
class Attack;

enum class ModuleReadyState : unsigned char
{
	Ready,
	Cooling,
};

namespace ModuleNodeKindFill
{
	inline constexpr Color kFill[] = {
		Color(255u, 255u,   0u, 255u), // Core      RGB(255,255,  0) #FFFF00 纯黄
		Color(255u, 180u,  60u, 255u), // Spawn     RGB(255,180, 60) #FFB43C 橙
		Color(120u, 200u, 255u, 255u), // Attribute RGB(120,200,255) #78C8FF 青
		Color(140u, 220u, 130u, 255u), // Rule      RGB(140,220,130) #8CDC82 绿
		Color(192u, 192u, 192u, 255u), // Passive   RGB(192,192,192) #C0C0C0 银
		Color(200u, 160u, 255u, 255u), // Other     RGB(200,160,255) #C8A0FF 紫
		Color(  0u,   0u,   0u,   0u), // Fusion    表槽；外观对半用子节点 Kind 色
	};

	static_assert(sizeof(kFill) / sizeof(kFill[0]) == ModuleNodeKindCount(), "ModuleNodeKindFill::kFill size must match ModuleNodeKindCount");
}

struct ModuleNodeLevel
{
	static constexpr int kMin = 1;
	static constexpr int kMax = 3;
	static constexpr std::size_t kCount = 3;

	int value{ kMin };

	// 把等级夹到 [kMin, kMax] 后写入。
	void Set(int level) noexcept
	{
		value = Clamp(level);
	}

	[[nodiscard]] int Get() const noexcept
	{
		return value;
	}

	// 当前等级对应的数组下标（0 = 1 级）。
	[[nodiscard]] std::size_t Index() const noexcept
	{
		return static_cast<std::size_t>(Get() - kMin);
	}

	// 把等级夹到 [kMin, kMax]。
	[[nodiscard]] static constexpr int Clamp(int level) noexcept
	{
		if (level < kMin)
		{
			return kMin;
		}
		if (level > kMax)
		{
			return kMax;
		}
		return level;
	}
};
static_assert(ModuleNodeLevel::kCount == static_cast<std::size_t>(ModuleNodeLevel::kMax - ModuleNodeLevel::kMin + 1));
static_assert(ModuleNodeLevel::Clamp(0) == ModuleNodeLevel::kMin);
static_assert(ModuleNodeLevel::Clamp(99) == ModuleNodeLevel::kMax);

// Node 买入造价。卖出价恒为买入的一半。不随等级自动变。
struct ModuleNodePrice
{
	int buy{ 4 };

	// 写入买入造价。小于等于 0 存 0。
	void SetBuy(int price) noexcept
	{
		buy = (price <= 0) ? 0 : price;
	}

	[[nodiscard]] constexpr int GetBuy() const noexcept
	{
		return buy;
	}

	[[nodiscard]] constexpr int GetSell() const noexcept
	{
		return buy / 2;
	}
};
static_assert(ModuleNodePrice{}.GetBuy() == 4);
static_assert(ModuleNodePrice{}.GetSell() == 2);

// Node 冷却：时长、剩余、Ready/Cooling。
struct ModuleNodeCooldown
{
	ModuleReadyState state{ ModuleReadyState::Ready };
	float duration{ 3.0f };
	float remaining{ 0.0f };

	// 写入冷却时长。小于 0 存 0。
	void SetDuration(float seconds) noexcept
	{
		duration = (seconds < 0.0f) ? 0.0f : seconds;
	}

	[[nodiscard]] float GetDuration() const noexcept
	{
		return duration;
	}

	[[nodiscard]] float GetRemaining() const noexcept
	{
		return remaining;
	}

	[[nodiscard]] ModuleReadyState GetState() const noexcept
	{
		return state;
	}

	[[nodiscard]] bool IsReady() const noexcept
	{
		return state == ModuleReadyState::Ready;
	}

	void Start() noexcept
	{
		state = ModuleReadyState::Cooling;
		remaining = duration;
	}

	void Reset() noexcept
	{
		state = ModuleReadyState::Ready;
		remaining = 0.0f;
	}

	void Tick(float dt) noexcept
	{
		if (state != ModuleReadyState::Cooling)
		{
			return;
		}
		remaining -= dt;
		if (remaining <= 0.0f)
		{
			remaining = 0.0f;
			state = ModuleReadyState::Ready;
		}
	}

	// 冷却剩余比例 [0, 1]；非冷却为 0。
	[[nodiscard]] float RemainRatio() const noexcept
	{
		if (state != ModuleReadyState::Cooling)
		{
			return 0.0f;
		}
		if (duration <= 1.0e-6f)
		{
			return 0.0f;
		}
		const float ratio = remaining / duration;
		if (ratio < 0.0f)
		{
			return 0.0f;
		}
		if (ratio > 1.0f)
		{
			return 1.0f;
		}
		return ratio;
	}
};

// 扫描波：最大半径与扩散速度。
struct ModuleNodeScan
{
	float maxRadius{ 10.0f };
	float expandSpeed{ 10.0f };

	void SetMaxRadius(float radius) noexcept
	{
		maxRadius = (radius < 0.0f) ? 0.0f : radius;
	}

	void SetExpandSpeed(float speed) noexcept
	{
		expandSpeed = (speed < 0.0f) ? 0.0f : speed;
	}

	[[nodiscard]] float GetMaxRadius() const noexcept
	{
		return maxRadius;
	}

	[[nodiscard]] float GetExpandSpeed() const noexcept
	{
		return expandSpeed;
	}
};

class IModuleNode
{
public:
	static constexpr unsigned kVisualSize = 16u;

	virtual ~IModuleNode() = default;

protected:
	IModuleNode() noexcept
		:
		instanceId_(++s_nextInstanceId_)
	{}

public:
	// ---- 身份 ----
	[[nodiscard]] std::uint32_t GetInstanceId() const noexcept { return instanceId_; }

protected:
	std::uint32_t instanceId_{ 0 };

public:
	// ---- 种类与装配：扫描命中时写入配方 ----
	virtual void ApplyTo(DeployContext& ctx) = 0;
	// 开火前仓内被动加算。默认空；Passive 节点覆写。
	virtual void ApplyWarehouseBonus(Attack& attack)
	{
		(void)attack;
	}
	[[nodiscard]] virtual ModuleNodeLabel GetModuleNodeLabel() const noexcept = 0;
	// 大类；默认 Other。未覆写的新节点会走 Other 色。
	[[nodiscard]] virtual ModuleNodeKind GetKind() const noexcept
	{
		return ModuleNodeKind::Other;
	}
	[[nodiscard]] bool IsCore() const noexcept { return GetKind() == ModuleNodeKind::Core; }

protected:
	[[nodiscard]] virtual Color GetReadyFillColor() const noexcept;

public:
	// ---- 等级：夹在 [1, 3]，SetLevel 会重算本类数值，不改造价 ----
	[[nodiscard]] int GetLevel() const noexcept
	{
		return level_.Get();
	}

	// 不在基类构造里调用（派生未完成时虚表不对）。
	void SetLevel(int level) noexcept
	{
		level_.Set(level);
		ApplyLevelStats_();
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

protected:
	virtual void ApplyLevelStats_() {}
	ModuleNodeLevel level_{};

public:
	// ---- 造价：默认买 4 / 卖 2；Core 为 0。炼成用 SetBuyPrice，不随 SetLevel ----
	[[nodiscard]] int GetBuyPrice() const noexcept
	{
		return price_.GetBuy();
	}

	void SetBuyPrice(int price) noexcept
	{
		price_.SetBuy(price);
	}

	[[nodiscard]] int GetSellPrice() const noexcept
	{
		return price_.GetSell();
	}

protected:
	ModuleNodePrice price_{};

public:
	// ---- 冷却：开火后 Cooling，Tick 结束回到 Ready ----
	[[nodiscard]] ModuleReadyState GetState() const noexcept { return cooldown_.GetState(); }
	[[nodiscard]] bool IsReady() const noexcept { return cooldown_.IsReady(); }
	[[nodiscard]] float GetCooldownRemaining() const noexcept { return cooldown_.GetRemaining(); }
	[[nodiscard]] float GetCooldownDuration() const noexcept { return cooldown_.GetDuration(); }
	void SetCooldownDuration(float seconds) noexcept { cooldown_.SetDuration(seconds); }

	void StartCooldown()
	{
		cooldown_.Start();
	}

	void ResetCooldown() noexcept
	{
		cooldown_.Reset();
	}

	void TickCooldown(float dt)
	{
		cooldown_.Tick(dt);
	}

protected:
	[[nodiscard]] float GetRemainRatio_() const noexcept;
	ModuleNodeCooldown cooldown_{};

public:
	// ---- 扫描波：Assembler 起环时读半径与扩散速度 ----
	[[nodiscard]] float GetScanMaxRadius() const noexcept { return scan_.GetMaxRadius(); }
	void SetScanMaxRadius(float radius) noexcept { scan_.SetMaxRadius(radius); }
	[[nodiscard]] float GetScanExpandSpeed() const noexcept { return scan_.GetExpandSpeed(); }
	void SetScanExpandSpeed(float speed) noexcept { scan_.SetExpandSpeed(speed); }

protected:
	ModuleNodeScan scan_{};

public:
	// ---- 场上位置与碰撞圆 ----
	[[nodiscard]] DirectX::XMFLOAT2 GetLocalPos() const noexcept { return localPos_; }
	void SetLocalPos(DirectX::XMFLOAT2 pos) noexcept { localPos_ = pos; }

	// 占用/点选跟残影：ghost 开着用地板原位，否则跟图标。
	[[nodiscard]] DirectX::XMFLOAT2 GetCollisionLocalPos() const noexcept
	{
		return layoutGhostActive_ ? layoutGhostLocalPos_ : localPos_;
	}

	[[nodiscard]] float GetHitRadius() const noexcept { return hitRadius_; }
	void SetHitRadius(float r) noexcept { hitRadius_ = r; }

	// 槽位残影 / 静置图标用半径；未覆盖时与命中半径相同。
	[[nodiscard]] float GetVisualRadius() const noexcept
	{
		return (visualRadiusOverride_ > 0.0f) ? visualRadiusOverride_ : hitRadius_;
	}

	// 当前跟随鼠标的图标半径。iconRadiusOverride_ 供炼成格按仓库比例缩放；否则 ghost 时用 hitRadius_（跟手），静置用 GetVisualRadius()。
	[[nodiscard]] float GetIconRadius() const noexcept
	{
		if (iconRadiusOverride_ > 0.0f)
		{
			return iconRadiusOverride_;
		}
		return layoutGhostActive_ ? hitRadius_ : GetVisualRadius();
	}

	// 覆盖绘制半径；传入 <= 0 等效于清除覆盖。
	void SetVisualRadiusOverride(float r) noexcept
	{
		visualRadiusOverride_ = r;
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

	void ClearVisualRadiusOverride() noexcept
	{
		visualRadiusOverride_ = 0.0f;
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

	// 炼成格内 Icon 半径覆盖；残影仍走 GetVisualRadius()。<=0 清除。
	void SetIconRadiusOverride(float r) noexcept
	{
		iconRadiusOverride_ = (r > 0.0f) ? r : 0.0f;
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

	void ClearIconRadiusOverride() noexcept
	{
		iconRadiusOverride_ = 0.0f;
		if (visualReady_)
		{
			ApplyVisualTransform_();
		}
	}

protected:
	DirectX::XMFLOAT2 localPos_{ 0.0f, 0.0f };
	float hitRadius_{ 10.0f };
	// <= 0 表示不覆盖，绘制走 hitRadius_。
	float visualRadiusOverride_{ 0.0f };
	// <= 0 表示不覆盖。炼成格 Icon 单独缩放，不放大场上残影。
	float iconRadiusOverride_{ 0.0f };

public:
	// ---- 绘制与拖放残影 ----
	virtual void InitVisual(Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 zoneOrigin);
	void SetZoneOrigin(DirectX::XMFLOAT3 zoneOrigin) noexcept;
	void SetZoneVisualScale(float scale) noexcept;
	void SyncVisual();
	void SubmitVisual();
	// 残影与图标分开交，整理态才能先画全区残影、再画全区图标。
	void SubmitGhost();
	void SubmitIcon();
	void BeginLayoutGhost(DirectX::XMFLOAT2 at) noexcept;
	void EndLayoutGhost() noexcept;
	[[nodiscard]] bool IsLayoutGhostActive() const noexcept { return layoutGhostActive_; }

protected:
	void ApplyVisualTransform_();
	void SyncMaskUV_();

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
