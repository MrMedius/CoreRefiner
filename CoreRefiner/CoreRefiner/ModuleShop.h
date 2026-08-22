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
	static constexpr int kColumns = 5;
	static constexpr std::size_t kSlotCount = static_cast<std::size_t>(kColumns);
	/** @brief 买卖框内沿相对商品排布区域的左右/上下留白；不决定卡片尺寸。 */
	static constexpr float kSlotMargin = 10.0f;
	/** @brief 商品卡固定宽度；与 kColumns 无关。 */
	static constexpr float kCardWidth = 140.0f;
	/** @brief 商品卡固定高度；与买卖框高度无关。 */
	static constexpr float kCardHeight = 220.0f;
	/** @brief 买卖框顶部 Gold / Reset 栏高度。 */
	static constexpr float kHudBarHeight = 32.0f;
	/** @brief 卡片顶边到图标圆的间隙。 */
	static constexpr float kCardIconPad = 8.0f;
	static constexpr float kPriceFontSize = 14.0f;
	/** @brief 商品在店内统一绘制半径；买走后恢复节点真实命中半径。 */
	static constexpr float kStoredVisualRadius = 15.0f;
	/** @brief 默认外壳半宽；Q4 起由 SetShellExtent 覆盖。 */
	static constexpr float kFrozenShellHalfX = 332.0f;
	/** @brief 默认外壳半高；Q4 起由 SetShellExtent 覆盖。 */
	static constexpr float kFrozenShellHalfY = 230.0f;
	/** @brief origin.y 到买卖框顶边的距离（Workbench 排版用，Q4 前保留）。 */
	static constexpr float kBoundsPad = 36.0f;
	/** @brief 外壳顶边相对 origin.y - kBoundsPad 再上扩的边距。 */
	static constexpr float kShellPadTop = 12.0f;
	/** @brief 外壳内沿相对外壳的边距。 */
	static constexpr float kShellInnerPad = 12.0f;
	/** @brief 买卖框与 Function 板、以及两块 Function 板之间的间隙。 */
	static constexpr float kShellInnerGap = 12.0f;
	/** @brief 外壳内竖直比例：买卖框。 */
	static constexpr int kTradeRatio = 2;
	/** @brief 外壳内竖直比例：每块 Function 板。 */
	static constexpr int kFunctionRatio = 1;

	struct Slot
	{
		std::unique_ptr<IModuleNode> node;
		bool sold{ false };
		int price{ 0 };
	};

	ModuleShop() = default;
	~ModuleShop() override = default;

	ModuleShop(const ModuleShop&) = delete;
	ModuleShop& operator=(const ModuleShop&) = delete;

	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Shop; }

	[[nodiscard]] std::size_t GetNodeCount() const noexcept override { return kSlotCount; }

	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override
	{
		return (index < kSlotCount) ? slots_[index].node.get() : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return origin_; }

	/** @brief 买卖框世界包围盒；拖入此范围才可出售。 */
	[[nodiscard]] BoundsWorld GetTradeBoundsWorld() const noexcept;
	/** @brief 整店外壳世界包围盒；只用于绘制，不参与命中。 */
	[[nodiscard]] BoundsWorld GetShellBoundsWorld() const noexcept override;
	/** @brief 命中用包围盒，与 GetTradeBoundsWorld 相同。 */
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;

	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;
	/** @brief 设置外壳半宽半高（世界像素）；origin 为外壳中心。 */
	void SetShellExtent(float halfX, float halfY) noexcept;

	void FillStock();

	[[nodiscard]] bool TryRefresh();

	void ResetVisit();

	void TickHud(float dt);
	void SyncHud();
	void SubmitHud();

	[[nodiscard]] bool HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	[[nodiscard]] int GetRefreshCost() const noexcept;

	void MarkSold(std::size_t index);

	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;

	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;

	void SubmitNodes() override;

	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;

	[[nodiscard]] DropResult EvalDrop(
		const IModuleNode& node,
		DirectX::XMFLOAT2 worldPos,
		ZoneId from) const noexcept override;

protected:
	void InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg) override;
	void SyncZoneTransforms_() override;
	void SubmitZoneBackground_() override;

private:
	[[nodiscard]] DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) const noexcept;
	/** @brief 外壳减去内边距后的内容矩形。 */
	[[nodiscard]] BoundsWorld ShellInnerRect_() const noexcept;
	/** @brief 2:1:1 切分的单位高度。 */
	[[nodiscard]] float SplitUnitHeight_() const noexcept;
	/** @brief 买卖框内商品卡中心的水平步长（按 kSlotCount 均分，不改变卡片尺寸）。 */
	[[nodiscard]] float SlotPitchX_() const noexcept;
	[[nodiscard]] float TradeHalfX_() const noexcept;
	[[nodiscard]] float TradeHalfY_() const noexcept;
	/** @brief 买卖框中心相对外壳中心的本地 Y。 */
	[[nodiscard]] float TradeCenterLocalY_() const noexcept;
	/** @brief 商品卡中心相对外壳中心的本地 Y（HUD 栏下方）。 */
	[[nodiscard]] float CardCenterLocalY_() const noexcept;
	void RelayoutSlots_();
	void RefreshSlotCards_();
	void EnsureSlotCardVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintSlotCard_(std::size_t index);
	void SyncSlotCardTransforms_() noexcept;
	void RerollStock_();
	void EnsureHudVisuals_();
	void PaintHudIcons_();
	void PaintHudNumber_(Canvas2D& canvas, int value, int& painted);
	void SyncHudTransforms_() noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 CurrencyIconCenter_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 RefreshButtonCenter_() const noexcept;
	[[nodiscard]] Color RefreshIconTint_() const noexcept;
	void EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintPanel_();
	void SyncPanelTransform_() noexcept;
	void EnsureReserveVisuals_(Graphics& gfx, Rgph::RenderGraph& rg);
	void PaintReservePanel_(std::size_t index);
	void SyncReserveTransforms_() noexcept;
	[[nodiscard]] BoundsWorld GetReserveBoundsWorld_(std::size_t index) const noexcept;
	[[nodiscard]] std::size_t FindSlotIndex_(const IModuleNode* node) const noexcept;

	std::array<Slot, kSlotCount> slots_{};
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> slotCards_{};
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT2 shellHalf_{ kFrozenShellHalfX, kFrozenShellHalfY };
	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	/** @brief 买卖框底板。 */
	std::unique_ptr<Canvas2D> panel_;
	/** @brief Function2 / Function3 占位板；仅绘制，不参与拖放。 */
	static constexpr std::size_t kReserveCount_ = 2;
	std::array<std::unique_ptr<Canvas2D>, kReserveCount_> reservePanels_{};

	static constexpr float kHudIconWorld_{ 24.0f };
	static constexpr float kHudHitPad_{ 4.0f };
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
};