#pragma once

#include "Canvas2D.h"
#include "Colors.h"
#include "ModuleNodeLabel.h"
#include "ModuleShopBlock.h"
#include "ModuleShopDraw.h"
#include "ModuleShopPanel.h"

#include <array>
#include <cstddef>
#include <memory>

// 商店买卖区：货架、货卡、锁、刷新和顶栏金额。
// 商店通过 trade_ 调用这里。格子位置按 bounds_ 相对 hostOrigin_ 计算。
class ModuleShopTrade : public ModuleShopBlock
{
public:
	// ---- 前置 ----
	// 货架列数。GetNode 的货架段长度跟这个走，不写死 5。
	static constexpr int kColumns = 5;
	static constexpr std::size_t kSlotCount = static_cast<std::size_t>(kColumns);
	// 货卡距离买卖框内边的空隙。
	static constexpr float kSlotMargin = 5.0f;
	static constexpr float kCardWidth = ModuleShopPanel::kWidth;
	static constexpr float kCardHeight = ModuleShopPanel::kHeight;
	// 顶栏高度。数值在 ModuleShopDraw::kTradeHudBarHeight。钱图标、刷新按钮和货卡纵向位置都用它。
	static constexpr float kHudBarHeight = ModuleShopDraw::kTradeHudBarHeight;
	// 顶栏数字的字号。数值在 ModuleShopDraw::kTradeHudFontSize。
	static constexpr float kPriceFontSize = ModuleShopDraw::kTradeHudFontSize;
	// 和仓库格相同的图标半径约定。货卡实际半径用 ModuleShopPanel::IconWorldRadius。
	static constexpr float kStoredVisualRadius = 15.0f;

	// 一格货。node 是还在卖的商品；卖掉后 node 为空。
	struct Slot
	{
		std::unique_ptr<IModuleNode> node;
		bool sold{ false };
		bool locked{ false };
		int price{ 0 };
	};

	ModuleShopTrade() = default;
	~ModuleShopTrade() override = default;

	ModuleShopTrade(const ModuleShopTrade&) = delete;
	ModuleShopTrade& operator=(const ModuleShopTrade&) = delete;

	// ---- 重载 ----
	// 拒绝倒计时走完后重画刷新图标，并刷新顶栏数字。
	void Tick(float dt) override;
	// 对齐买卖底板、货卡、锁和顶栏。
	void Sync() override;
	// 买卖底板和货卡，画在 Node 下面。
	void Submit() const override;
	// 顶栏和锁按钮，画在 Node 上面。
	void SubmitOverlay() const override;
	// 换语言后重画货卡文字。
	void RefreshCopy() override;

	// 商品的 unique_ptr 是否在某个货格里。
	[[nodiscard]] bool Owns(const IModuleNode* node) const noexcept override;
	// 从货格取出商品。不是货架上的返回空。
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;
	// 点到货卡上的商品时返回它，并把距离平方写到 outDistSq。
	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;
	// 固定交出全部货格，空位也算。长度是 kSlotCount。
	[[nodiscard]] std::size_t GetZoneNodeCount() const noexcept override;
	// 第 index 格的商品。空位返回空。下标只在货架内计。
	[[nodiscard]] IModuleNode* GetZoneNode(std::size_t index) const noexcept override;
	// 提交还在货格里的商品。
	void SubmitNodes() override;
	// 把货格里的商品对齐到 hostOrigin_ 并同步画面。
	void SyncNodes() override;

	// ---- 独有 · 货架 ----
	// 按可刷表给每个货格补一件商品。画面还没建好时先不建 Node 的图。
	void FillStock();
	// 清掉本局刷新次数，再重抽未锁定的货。
	void BeginVisit();
	// 刷新次数和拒绝倒计时归零。画面已建好时重画顶栏。
	void ResetVisit();
	// 钱够且还有可重抽的货时扣费并重抽。失败时刷新图标红一下。
	[[nodiscard]] bool TryRefresh();
	// 本次刷新要花的钱。次数越多越贵。
	[[nodiscard]] int GetRefreshCost() const noexcept;
	// 这一格卖掉：清掉商品和锁定，并重画货卡。
	void MarkSold(std::size_t index);
	// 购买没完成时，商品回到原来的货槽，不卖掉。
	void RestoreShelfNode(std::unique_ptr<IModuleNode> node, std::size_t index);
	// 这颗 Node 在第几格。不在货架上时返回 kSlotCount。
	[[nodiscard]] std::size_t FindSlotIndex(const IModuleNode* node) const noexcept;

	// ---- 独有 · 点击 ----
	// 点是否落在刷新按钮上。
	[[nodiscard]] bool HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点是否落在某个还在卖的货的锁按钮上。
	[[nodiscard]] bool HitLockButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点在锁上就切换锁定。没点中返回 false。
	bool ToggleLockAt(DirectX::XMFLOAT2 worldPos);

	// ---- 独有 · 位置 ----
	// 第 index 格商品相对 hostOrigin_ 的 localPos。
	[[nodiscard]] DirectX::XMFLOAT2 SlotLocalPos(std::size_t index) const;
	// 外壳尺寸变了之后，按新的买卖框把商品摆回货格并重画货卡。
	void Relayout();
	// 重画顶栏金额、刷新价格和图标颜色。
	void SyncHud();

private:
	// ---- 重载 ----
	// Ensure 记下画面设备后，按 bounds_ 建买卖底板、货卡和锁。
	void EnsureVisuals_() override;

	// ---- 独有 · 布局 ----
	// 货卡图标的世界半径。
	[[nodiscard]] float SlotCardIconRadius_() const;
	// 相邻两张货卡中心的水平距离。按买卖框宽度均分。
	[[nodiscard]] float SlotPitchX_() const noexcept;
	// 买卖框中心相对 hostOrigin_ 的偏移。
	[[nodiscard]] DirectX::XMFLOAT2 TradeCenterLocal_() const noexcept;
	// 第 index 张货卡中心相对 hostOrigin_ 的横向位置。
	[[nodiscard]] float CardCenterLocalX_(std::size_t index) const noexcept;
	// 货卡中心相对 hostOrigin_ 的纵向位置。顶栏下面留出 kSlotMargin。
	[[nodiscard]] float CardCenterLocalY_() const noexcept;

	// ---- 独有 · 货架 ----
	// 第一次刷新的价格，以及每多刷新一次加的钱。
	static constexpr int kRefreshBaseCost_{ 1 };
	static constexpr int kRefreshCostStep_{ 1 };
	// 按当前 bounds_ 把商品摆回货格，并重画货卡。
	void RelayoutSlots_();
	// 用指定 Label 补第 index 格。画面已建好时顺便建商品的图。
	void RestockSlot_(std::size_t index, ModuleNodeLabel label);
	// 是否还有没被锁定、可以重抽的货格。
	[[nodiscard]] bool HasRerollableSlot_() const noexcept;
	// 刷新失败：图标红一小段时间。
	void DenyRefresh_();
	// 保留锁定的货，其余从可刷表里重抽。
	void RerollStock_();

	// ---- 独有 · 顶栏 ----
	// 钱图标的世界中心。在顶栏左端。图标边长和点击放宽见 ModuleShopDraw 的买卖段。
	[[nodiscard]] DirectX::XMFLOAT2 CurrencyIconCenter_() const noexcept;
	// 刷新按钮的世界中心。在顶栏右端。
	[[nodiscard]] DirectX::XMFLOAT2 RefreshButtonCenter_() const noexcept;
	// 刷新图标颜色：拒绝时红，钱不够时灰，否则绿。
	[[nodiscard]] Color RefreshIconTint_() const noexcept;
	// 钱图标、刷新图标和两段数字的画布。已有就不重建。
	void EnsureHudVisuals_();
	// 钱图标画一次。刷新图标只在颜色变化时重画。
	void PaintHudIcons_();
	// 数字变了才重画。painted 记下上次画过的值。
	void PaintHudNumber_(Canvas2D& canvas, int value, int& painted);
	// 把顶栏图标和数字摆到当前买卖框上。
	void SyncHudTransforms_() noexcept;

	// ---- 独有 · 货卡和锁 ----
	// 第 index 格锁按钮的世界中心。在货卡下边。空隙和边长见 ModuleShopDraw 的买卖段。
	[[nodiscard]] DirectX::XMFLOAT2 LockButtonCenter_(std::size_t index) const noexcept;
	// 点中的锁属于第几格。没点中返回 kSlotCount。
	[[nodiscard]] std::size_t HitLockSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 按当前货格重画货卡和锁，并刷新顶栏。
	void RefreshSlotCards_();
	// 每格一张货卡画布。已有就不重建。
	void EnsureSlotCardVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	// 把货卡中心对齐到当前买卖框。
	void SyncSlotCardTransforms_() noexcept;
	// 每格一把锁的画布。已有就不重建。
	void EnsureLockButtonVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	// 按锁定状态画这一格的锁。卖掉或空位则清空。
	void PaintLockButton_(std::size_t index);
	// 把锁按钮对齐到货卡下边。
	void SyncLockButtonTransforms_() noexcept;

	// ---- 独有 · 底板 ----
	// 按买卖框尺寸建底板。尺寸变了才重画。
	void EnsureTradePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	// 底板底色，再在顶上画一条顶栏。
	void PaintTradePanel_();
	// 把底板对齐到当前买卖框。
	void SyncTradePanelTransform_() noexcept;

	// ---- 数据 · 货架 ----
	// 全部货格。长度是 kSlotCount。
	std::array<Slot, kSlotCount> slots_{};

	// ---- 数据 · 底板 ----
	// 买卖底板，画在商品下面。
	std::unique_ptr<Canvas2D> tradePanel_;

	// ---- 数据 · 货卡和锁 ----
	// 每格一张货卡。
	std::array<ModuleShopPanel, kSlotCount> slotPanels_{};
	// 每格一个锁按钮，画在商品上面。
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> lockButtons_{};

	// ---- 数据 · 顶栏 ----
	// 本局已经成功刷新的次数。
	int refreshCount_{ 0 };
	// 刷新被拒绝后，图标保持红色的剩余秒数。
	float refreshDeniedSec_{ 0.0f };
	// 上次画过的金额和刷新价格。-1 表示还没画。
	int paintedCurrency_{ -1 };
	int paintedRefreshCost_{ -1 };
	// 上次画过的刷新图标颜色。
	Color paintedRefreshTint_{};
	// 钱图标只画一次。
	bool currencyIconReady_{ false };
	std::unique_ptr<Canvas2D> currencyIcon_;
	std::unique_ptr<Canvas2D> refreshIcon_;
	std::unique_ptr<Canvas2D> currencyText_;
	std::unique_ptr<Canvas2D> refreshCostText_;
};
