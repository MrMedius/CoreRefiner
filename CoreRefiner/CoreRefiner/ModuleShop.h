#pragma once
#include "Canvas2D.h"
#include "Colors.h"
#include "IModuleNode.h"
#include "IModuleZone.h"

#include <array>
#include <cstddef>
#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ModuleShop : public IModuleZone
{
public:
	ModuleShop() = default;
	~ModuleShop() override = default;

	ModuleShop(const ModuleShop&) = delete;
	ModuleShop& operator=(const ModuleShop&) = delete;

	// ---- 外壳：整店外框与 2:1:1 竖切（买卖 / 炼成 / Function3）----
	static constexpr float kFrozenShellHalfX = 332.0f;
	static constexpr float kFrozenShellHalfY = 230.0f;
	// origin.y 到买卖框顶边的距离（Workbench 排版用，Q4 前保留）。
	static constexpr float kBoundsPad = 36.0f;
	static constexpr float kShellPadTop = 12.0f;
	static constexpr float kShellInnerPad = 12.0f;
	static constexpr float kShellInnerGap = 12.0f;
	static constexpr int kTradeRatio = 2;
	static constexpr int kFunctionRatio = 1;
	static constexpr std::size_t kFunctionBandCount_ = 2;

	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Shop; }
	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return origin_; }
	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;
	void SetShellExtent(float halfX, float halfY) noexcept;
	[[nodiscard]] BoundsWorld GetShellBoundsWorld() const noexcept override;

protected:
	void InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg) override;
	void SyncZoneTransforms_() override;
	void SubmitZoneBackground_() override;

private:
	[[nodiscard]] BoundsWorld ShellInnerRect_() const noexcept;
	[[nodiscard]] float SplitUnitHeight_() const noexcept;
	// 三块内容板共用的半宽，与买卖框相同。
	[[nodiscard]] float ContentHalfX_() const noexcept;
	[[nodiscard]] BoundsWorld FunctionBandBounds_(std::size_t band) const noexcept;

	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT2 shellHalf_{ kFrozenShellHalfX, kFrozenShellHalfY };
	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };

	// ---- 买卖区 ----
public:
	static constexpr int kColumns = 5;
	static constexpr std::size_t kSlotCount = static_cast<std::size_t>(kColumns);
	static constexpr float kSlotMargin = 10.0f;
	static constexpr float kCardWidth = 140.0f;
	static constexpr float kCardHeight = 220.0f;
	static constexpr float kHudBarHeight = 32.0f;
	static constexpr float kCardIconPad = 8.0f;
	static constexpr float kPriceFontSize = 14.0f;
	static constexpr float kStoredVisualRadius = 15.0f;

	struct Slot
	{
		std::unique_ptr<IModuleNode> node;
		bool sold{ false };
		bool locked{ false };
		int price{ 0 };
	};

	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return kSlotCount; }
	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		return (index < kSlotCount) ? slots_[index].node.get() : nullptr;
	}

	[[nodiscard]] BoundsWorld GetTradeBoundsWorld() const noexcept;
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;
	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	void FillStock();
	void BeginVisit();
	[[nodiscard]] bool TryRefresh();
	void ResetVisit();
	void RefreshCopy();
	void TickHud(float dt);
	void SyncHud();
	void SubmitHud();
	[[nodiscard]] bool HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] bool HitLockButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	bool ToggleLockAt(DirectX::XMFLOAT2 worldPos);
	[[nodiscard]] int GetRefreshCost() const noexcept;
	void MarkSold(std::size_t index);

	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;
	void SubmitNodes() override;
	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;
	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept override;

private:
	// 买卖 · 逻辑
	[[nodiscard]] DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) const noexcept;
	[[nodiscard]] float SlotPitchX_() const noexcept;
	[[nodiscard]] float TradeHalfY_() const noexcept;
	[[nodiscard]] float TradeCenterLocalY_() const noexcept;
	[[nodiscard]] float CardCenterLocalY_() const noexcept;
	[[nodiscard]] std::size_t FindSlotIndex_(const IModuleNode* node) const noexcept;
	void RelayoutSlots_();
	void RestockSlot_(std::size_t index, ModuleNodeLabel label);
	[[nodiscard]] bool HasRerollableSlot_() const noexcept;
	void DenyRefresh_();
	void RerollStock_();
	[[nodiscard]] DirectX::XMFLOAT2 CurrencyIconCenter_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 RefreshButtonCenter_() const noexcept;
	[[nodiscard]] Color RefreshIconTint_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 LockButtonCenter_(std::size_t index) const noexcept;
	[[nodiscard]] std::size_t HitLockSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;

	// 买卖 · 绘制
	void EnsureTradePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintTradePanel_();
	void SyncTradePanelTransform_() noexcept;
	void RefreshSlotCards_();
	void EnsureSlotCardVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintSlotCard_(std::size_t index);
	void SyncSlotCardTransforms_() noexcept;
	void EnsureLockButtonVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintLockButton_(std::size_t index);
	void SyncLockButtonTransforms_() noexcept;
	void EnsureHudVisuals_();
	void PaintHudIcons_();
	void PaintHudNumber_(Canvas2D& canvas, int value, int& painted);
	void SyncHudTransforms_() noexcept;

	std::array<Slot, kSlotCount> slots_{};
	std::unique_ptr<Canvas2D> tradePanel_;
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> slotCards_{};
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> lockButtons_{};

	static constexpr float kHudIconWorld_{ 24.0f };
	static constexpr float kHudHitPad_{ 4.0f };
	static constexpr float kLockButtonGap_{ 6.0f };
	static constexpr unsigned kLockButtonPixels_{ 48u };
	static constexpr float kLockButtonWorld_{ 36.0f };
	static constexpr unsigned kLockBorderTexels_{ 4u };
	static constexpr unsigned kLockIconScale_{ 2u };
	static constexpr int kRefreshBaseCost_{ 1 };
	static constexpr int kRefreshCostStep_{ 1 };

	int refreshCount_{ 0 };
	float refreshDeniedSec_{ 0.0f };
	int paintedCurrency_{ -1 };
	int paintedRefreshCost_{ -1 };
	Color paintedRefreshTint_{};
	bool currencyIconReady_{ false };
	std::unique_ptr<Canvas2D> currencyIcon_;
	std::unique_ptr<Canvas2D> refreshIcon_;
	std::unique_ptr<Canvas2D> currencyText_;
	std::unique_ptr<Canvas2D> refreshCostText_;

	// ---- 炼成区 ----
public:
	// 炼成停放：不接管所有权。0=素材 1=主体；未命中返回 3。
	[[nodiscard]] std::size_t HitRefineParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 RefineSlotWorldCenter(std::size_t slot) const noexcept;
	[[nodiscard]] bool CanParkRefine(const IModuleNode& node, std::size_t slot) const noexcept;
	void ParkRefine(std::size_t slot, IModuleNode& node);
	void UnbindRefine(IModuleNode* node) noexcept;
	void ClearRefineParks();
	[[nodiscard]] bool IsRefineParked(const IModuleNode* node) const noexcept;
	// 格内 Icon 按仓库格比例缩放；残影半径仍走 GetVisualRadius()。
	void ApplyRefineParkIcon(IModuleNode& node) noexcept;

private:
	static constexpr std::size_t kRefineSlotCount_ = 3;
	static constexpr std::size_t kRefineButtonCount_ = 4;
	static constexpr float kRefineSlotLabelFont_ = 20.0f;

	[[nodiscard]] BoundsWorld GetRefineBoundsWorld_() const noexcept;
	[[nodiscard]] std::size_t HitRefineSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] bool RefineWorldToPixel_(DirectX::XMFLOAT2 world, float& px, float& py) const noexcept;
	[[nodiscard]] float RefineSlotIconRadius_() const noexcept;
	void EjectRefineOccupant_(IModuleNode& occupant);

	void EnsureRefineVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintRefinePanel_();
	void SyncRefineTransform_() noexcept;

	std::unique_ptr<Canvas2D> refinePanel_;
	std::array<IModuleNode*, kRefineSlotCount_> refineParked_{};

	// ---- Function3：占位，宽度与买卖框对齐 ----
private:
	[[nodiscard]] BoundsWorld GetFunction3BoundsWorld_() const noexcept;

	void EnsureFunction3Visuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintFunction3Panel_();
	void SyncFunction3Transform_() noexcept;

	std::unique_ptr<Canvas2D> function3Panel_;
};
