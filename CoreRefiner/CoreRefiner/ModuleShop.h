#pragma once
#include "Canvas2D.h"
#include "Colors.h"
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleShopPanel.h"

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
	static constexpr float kSlotMargin = 5.0f;
	static constexpr float kCardWidth = ModuleShopPanel::kWidth;
	static constexpr float kCardHeight = ModuleShopPanel::kHeight;
	static constexpr float kHudBarHeight = 32.0f;
	static constexpr float kPriceFontSize = 14.0f;
	static constexpr float kStoredVisualRadius = 15.0f;

	struct Slot
	{
		std::unique_ptr<IModuleNode> node;
		bool sold{ false };
		bool locked{ false };
		int price{ 0 };
	};

	// 货槽 + 结果 + 素材店有 + 主体店有。买卖勿把 GetNodeCount 当货槽数。
	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return kSlotCount + 3; }
	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		if (index < kSlotCount)
		{
			return slots_[index].node.get();
		}
		if (index == kSlotCount)
		{
			return refineResult_.get();
		}
		if (index == kSlotCount + 1)
		{
			return refineFeedOwned_[0].get();
		}
		if (index == kSlotCount + 2)
		{
			return refineFeedOwned_[1].get();
		}
		return nullptr;
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
	[[nodiscard]] DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) const;
	[[nodiscard]] float SlotCardIconRadius_() const;
	[[nodiscard]] float SlotPitchX_() const noexcept;
	[[nodiscard]] float TradeHalfY_() const noexcept;
	[[nodiscard]] float TradeCenterLocalY_() const noexcept;
	[[nodiscard]] float CardCenterLocalX_(std::size_t index) const noexcept;
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
	std::array<ModuleShopPanel, kSlotCount> slotPanels_{};
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> lockButtons_{};

	static constexpr float kHudIconWorld_{ 24.0f };
	static constexpr float kHudHitPad_{ 4.0f };
	static constexpr float kLockButtonGap_{ 4.0f };
	static constexpr unsigned kLockButtonPixels_{ 48u };
	static constexpr float kLockButtonWorld_{ 30.0f };
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
	// 炼成停放：0=素材 1=主体 2=结果；未命中返回 3。场/仓不接管；店有 Node 由 unique_ptr 持有。
	[[nodiscard]] std::size_t HitRefineParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 RefineSlotWorldCenter(std::size_t slot) const noexcept;
	[[nodiscard]] bool CanParkRefine(const IModuleNode& node, std::size_t slot) const noexcept;
	// owner 用来把格子世界中心写成 localPos（场/仓/店）。
	void ParkRefine(std::size_t slot, IModuleNode& node, IModuleZone* owner);
	void UnbindRefine(IModuleNode* node) noexcept;
	void ClearRefineParks();
	void ClearRefineResult();
	void RestoreRefineResult(std::unique_ptr<IModuleNode> node);
	void RestoreRefineHeld(std::unique_ptr<IModuleNode> node, std::size_t slot);
	[[nodiscard]] std::size_t FindRefineParkIndex(const IModuleNode* node) const noexcept;
	[[nodiscard]] bool IsRefineParked(const IModuleNode* node) const noexcept;
	[[nodiscard]] bool IsRefineResultParked(const IModuleNode* node) const noexcept;
	[[nodiscard]] bool IsShopRefineOwned(const IModuleNode* node) const noexcept;
	// 格内 Icon 按仓库格比例缩放；素材/主体残影仍走来源尺寸。
	void ApplyRefineParkIcon(IModuleNode& node) noexcept;
	// 店有炼成 Node：残影钉在结果格（格内尺寸），跟手图标另清 Icon 覆盖。
	void PinShopRefineHomeGhost(IModuleNode& node) noexcept;
	[[nodiscard]] bool HitRefineButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	bool TryClickRefine(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse);

private:
	static constexpr std::size_t kRefineSlotCount_ = 3;
	static constexpr std::size_t kRefineButtonCount_ = 4;
	static constexpr float kRefineSlotLabelFont_ = 20.0f;
	static constexpr int kRefineOpCost_{ 1 };

	struct RefineLayout_
	{
		BoundsWorld bounds{};
		int cw{ 1 };
		int ch{ 1 };
		int pad{ 0 };
		int side{ 0 };
		int slotTop{ 0 };
		int slotXs[3]{};
		int btnW{ 120 };
		int btnH{ 14 };
		int btnX{ 0 };
		int btnTop{ 0 };
		int btnGap{ 5 };
	};
	[[nodiscard]] BoundsWorld GetRefineBoundsWorld_() const noexcept;
	[[nodiscard]] RefineLayout_ CurrentRefineLayout_() const noexcept;
	static void FillRefineLayout_(RefineLayout_& layout) noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 RefineSlotWorldCenterFrom_(const RefineLayout_& layout, std::size_t slot) const noexcept;
	[[nodiscard]] float RefineSlotIconRadiusFrom_(const RefineLayout_& layout) const noexcept;
	[[nodiscard]] std::size_t HitRefineSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] std::size_t HitRefineButtonIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] bool RefineWorldToPixel_(DirectX::XMFLOAT2 world, const RefineLayout_& layout, float& px, float& py) const noexcept;
	[[nodiscard]] float RefineSlotIconRadius_() const noexcept;
	void PlaceRefinePark_(std::size_t slot, IModuleNode& node, IModuleZone* owner);
	// 按来源离开素材/主体格：店有回结果栏；场/仓回残影。
	void BounceRefinePark_(IModuleNode& node);
	[[nodiscard]] bool HasRefinePairReady_() const noexcept;
	[[nodiscard]] bool CanUpgradeRefine_() const noexcept;
	[[nodiscard]] bool CanFuseRefine_() const noexcept;
	[[nodiscard]] bool CanEvolveRefine_() const noexcept;
	[[nodiscard]] bool CanReturnRefine_() const noexcept;
	[[nodiscard]] IModuleZone* FindRefineOwner_(IModuleNode* node, IModuleZone* field, IModuleZone* warehouse) const noexcept;
	struct RefineTakenPair_
	{
		std::unique_ptr<IModuleNode> material;
		std::unique_ptr<IModuleNode> subject;
		IModuleZone* materialOwner{ nullptr };
		IModuleZone* subjectOwner{ nullptr };
	};
	bool TryTakeRefinePair_(IModuleZone* field, IModuleZone* warehouse, RefineTakenPair_& out);
	bool TryUpgradeRefine_(IModuleZone* field, IModuleZone* warehouse);
	bool TryFuseRefine_(IModuleZone* field, IModuleZone* warehouse);
	bool TryEvolveRefine_(IModuleZone* field, IModuleZone* warehouse);
	bool TryReturnRefine_();
	void AdoptRefineResult_(std::unique_ptr<IModuleNode> node);
	void PlaceRefineResultVisual_();
	void RelocateShopOwnedToFeed_(std::size_t slot, IModuleNode& node);
	bool ReturnShopOwnedToResult_(IModuleNode& node);

	void EnsureRefineVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintRefinePanel_();
	void SyncRefineTransform_() noexcept;

	std::unique_ptr<Canvas2D> refinePanel_;
	std::array<IModuleNode*, kRefineSlotCount_> refineParked_{};
	std::unique_ptr<IModuleNode> refineResult_{};
	// 停在素材/主体的店有 Node；与 refineResult_ 互斥，至多一颗。
	std::array<std::unique_ptr<IModuleNode>, 2> refineFeedOwned_{};
	bool paintedUpgradeLit_{ false };
	bool paintedFuseLit_{ false };
	bool paintedEvolveLit_{ false };
	bool paintedReturnLit_{ false };
	int paintedRefineCurrency_{ -1 };

	// ---- Function3：占位，宽度与买卖框对齐 ----
private:
	[[nodiscard]] BoundsWorld GetFunction3BoundsWorld_() const noexcept;

	void EnsureFunction3Visuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintFunction3Panel_();
	void SyncFunction3Transform_() noexcept;

	std::unique_ptr<Canvas2D> function3Panel_;
};

// 这颗 Node 现在归谁。店有炼成必须先于场/仓 FindNodeIndex，否则结果格再炼会认错主人。
[[nodiscard]] inline IModuleZone* FindNodeOwnerZone(
	const IModuleNode* node,
	IModuleZone* const* zones,
	std::size_t count) noexcept
{
	if (node == nullptr || zones == nullptr)
	{
		return nullptr;
	}
	const std::size_t npos = static_cast<std::size_t>(-1);
	for (std::size_t i = 0; i < count; ++i)
	{
		IModuleZone* zone = zones[i];
		if (zone == nullptr)
		{
			continue;
		}
		if (const auto* shop = dynamic_cast<const ModuleShop*>(zone);
			shop != nullptr && shop->IsShopRefineOwned(node))
		{
			return zone;
		}
	}
	for (std::size_t i = 0; i < count; ++i)
	{
		IModuleZone* zone = zones[i];
		if (zone != nullptr && zone->FindNodeIndex(node) != npos)
		{
			return zone;
		}
	}
	return nullptr;
}