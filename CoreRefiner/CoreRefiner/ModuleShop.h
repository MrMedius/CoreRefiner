#pragma once
#include "Canvas2D.h"
#include "Colors.h"
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "NodeInfoPanel.h"

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
	static constexpr int kRows = 1;
	static constexpr std::size_t kSlotCount = static_cast<std::size_t>(kColumns) * static_cast<std::size_t>(kRows);
	static constexpr float kSlotPitch = 140.0f;
	static constexpr float kBoundsPad = 36.0f;
	static constexpr float kInfoMaxWidthPx = 140.0f;
	static constexpr float kPriceFontSize = 14.0f;
	static constexpr float kPriceGapBelowIcon = 2.0f;
	/** @brief 商品在店内统一绘制半径；买走后恢复节点真实命中半径。 */
	static constexpr float kStoredVisualRadius = 15.0f;
	/** @brief 整店外壳相对买卖框左右各扩的边距。 */
	static constexpr float kShellPadX = 16.0f;
	/** @brief 整店外壳相对买卖框顶边再扩的边距。 */
	static constexpr float kShellPadTop = 12.0f;
	/** @brief Function2 / Function3 各预留的高度（S2 再画板）。 */
	static constexpr float kReservePanelHeight = 96.0f;
	/** @brief 买卖框与预留板、以及两块预留板之间的间隙。 */
	static constexpr float kReserveGap = 12.0f;

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

	[[nodiscard]] const Slot* GetSlot(std::size_t index) const noexcept
	{
		return (index < kSlotCount) ? &slots_[index] : nullptr;
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return origin_; }

	/** @brief 买卖框世界包围盒；拖入此范围才可出售。 */
	[[nodiscard]] BoundsWorld GetTradeBoundsWorld() const noexcept;
	/** @brief 整店外壳世界包围盒；只用于绘制，不参与命中。 */
	[[nodiscard]] BoundsWorld GetShellBoundsWorld() const noexcept override;
	/** @brief 与 GetTradeBoundsWorld 相同，保留给现有调用点。 */
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;

	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;

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
	void SubmitInfoPanels();
	void SubmitAllVisuals();

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
	[[nodiscard]] static DirectX::XMFLOAT2 SlotLocalPos_(std::size_t index) noexcept;
	[[nodiscard]] static float HalfSpanX_() noexcept;
	[[nodiscard]] static float HalfSpanY_() noexcept;
	void RelayoutSlots_();
	void SyncInfoPanels_();
	void EnsurePriceVisuals_();
	void PaintPrice_(std::size_t index);
	void SyncPriceLabels_();
	void SubmitPrices_();
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
	[[nodiscard]] std::size_t FindSlotIndex_(const IModuleNode* node) const noexcept;

	std::array<Slot, kSlotCount> slots_{};
	std::array<NodeInfoPanel, kSlotCount> infoPanels_{};
	std::array<std::unique_ptr<Canvas2D>, kSlotCount> priceCanvases_{};
	std::array<int, kSlotCount> paintedPrice_{};
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	/** @brief 买卖框底板（含槽格线）。 */
	std::unique_ptr<Canvas2D> panel_;

	static constexpr float kHudIconWorld_{ 24.0f };
	static constexpr float kHudHitPad_{ 4.0f };
	static constexpr int kRefreshBaseCost_{ 5 };
	static constexpr int kRefreshCostStep_{ 5 };

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