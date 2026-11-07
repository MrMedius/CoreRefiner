#pragma once
#include "Canvas2D.h"
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleShopBlock.h"
#include "ModuleShopTrade.h"
#include "ModuleShopUltra.h"

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

	// ---- 外壳：整店外框与 2:1:1 竖切（买卖 / 炼成 / 奥义）----
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

	// ---- 买卖区。货架、锁、刷新和顶栏在 trade_，这里只转发。----
public:
	// 货架列数和格数。和买卖区是同一份，编辑器靠 kSlotCount 认出货架下标。
	static constexpr int kColumns = ModuleShopTrade::kColumns;
	static constexpr std::size_t kSlotCount = ModuleShopTrade::kSlotCount;

	// [0, kSlotCount) 货架，kSlotCount 是炼成结果，后面两格是店有炼成 Node。再往后由奥义区按自己的下标交出。
	[[nodiscard]] std::size_t GetNodeCount() const noexcept override;
	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override;
	[[nodiscard]] ModuleShopUltra& GetUltraArea() noexcept;
	[[nodiscard]] const ModuleShopUltra& GetUltraArea() const noexcept;

	// 买卖框的世界矩形。外壳按 2:1:1 切出来，交给 trade_ 当 bounds。
	[[nodiscard]] BoundsWorld GetTradeBoundsWorld() const noexcept;
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;
	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

	// 按可刷表给货架补货。画面已建好时，买卖区自己给商品建图。
	void FillStock();
	// 清掉本局刷新次数，再重抽未锁定的货。
	void BeginVisit();
	// 结束本局时清空奥义区。再次打开商店不调用它。
	void ClearUltra();
	// 钱够且还有可重抽的货时，交给买卖区扣费并重抽。
	[[nodiscard]] bool TryRefresh();
	// 刷新次数和拒绝倒计时归零。
	void ResetVisit();
	// 换语言：买卖区重画货卡，商店重画炼成板。
	void RefreshCopy();
	// 炼成按钮亮灭仍在商店里算。刷新倒计时和顶栏交给买卖区。
	void TickHud(float dt);
	// 重画顶栏金额和刷新价格。
	void SyncHud();
	// 提交顶栏和锁按钮，画在 Node 上面。
	void SubmitHud();
	// 点是否落在刷新按钮上。
	[[nodiscard]] bool HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点是否落在某个还在卖的货的锁按钮上。
	[[nodiscard]] bool HitLockButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点在锁上就切换锁定。
	bool ToggleLockAt(DirectX::XMFLOAT2 worldPos);
	// 本次刷新要花的钱。
	[[nodiscard]] int GetRefreshCost() const noexcept;
	// 这一格卖掉：清掉商品和锁定。
	void MarkSold(std::size_t index);
	// 购买没完成时，商品回到原来的货槽，不卖掉。
	void RestoreShelfNode(std::unique_ptr<IModuleNode> node, std::size_t index);

	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;
	void SubmitNodes() override;
	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;
	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept override;

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
	// 奥义本体和内部栏里的 Node，含滚出画面的。
	[[nodiscard]] bool IsShopUltraOwned(const IModuleNode* node) const noexcept;
	// 商店悬停弹 Panel：炼成停放、奥义本体、内部栏。货架不弹。
	[[nodiscard]] bool ShowsHoverInfo(const IModuleNode* node) const noexcept
	{
		return IsRefineParked(node) || IsShopUltraOwned(node);
	}
	// 只认奥义栏里的那一颗，不含内部栏。
	[[nodiscard]] bool IsUltraBody(const IModuleNode* node) const noexcept;
	// 残影留在奥义栏。跟手图标由调用方清掉 Icon 覆盖。
	void PinUltraHomeGhost(IModuleNode& node) noexcept;
	// 拖出失败时放回奥义栏。
	void RestoreUltra(std::unique_ptr<IModuleNode> node);
	// 内部栏下标。不在里面时返回 npos。
	[[nodiscard]] std::size_t FindUltraInnerIndex(const IModuleNode* node) const noexcept;
	// 拖出失败时放回这个内部栏。
	void RestoreUltraInner(std::unique_ptr<IModuleNode> node, std::size_t index);
	// 只接奥义区落点。对不上就原样退回，不进货架、不卖掉。
	bool TryAcceptUltraDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos);
	// 还在货架上的商品。
	[[nodiscard]] bool IsShelfStock(const IModuleNode* node) const noexcept;
	// 点在奥义区整块里。
	[[nodiscard]] bool HitUltraArea(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 从货架买进内部栏。失败时商品回到原货槽。
	bool TryPurchaseShelfIntoUltra(IModuleNode* node, DirectX::XMFLOAT2 localPos);
	// 从奥义区卖到买卖框。奥义本体连同内部 Node 一起销毁。
	void SellFromUltra(IModuleNode* node);
	// 拖到买卖框时会卖掉。货架上的商品和核心不算。
	[[nodiscard]] bool SellsOnTradeDrop(const IModuleNode& node) const noexcept;
	// 格内 Icon 按仓库格比例缩放；素材/主体残影仍走来源尺寸。
	void ApplyRefineParkIcon(IModuleNode& node) noexcept;
	// 店有炼成 Node：残影钉在结果格（格内尺寸），跟手图标另清 Icon 覆盖。
	void PinShopRefineHomeGhost(IModuleNode& node) noexcept;
	[[nodiscard]] bool HitRefineButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	bool TryClickRefine(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse);
	// 奥义区按钮。编辑器只问有没有点到，锻造、打孔和翻页由奥义区自己分。
	[[nodiscard]] bool HitUltraButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	bool TryClickUltra(DirectX::XMFLOAT2 worldPos);
	// 这次点的是更名时返回 true。调用后记号消失。
	bool TakeUltraRenameRequest() noexcept;
	// 这次点的是重绘时返回 true。调用后记号消失。
	bool TakeUltraRedrawRequest() noexcept;
	// 鼠标在奥义图标上时返回它，给 Panel 用。选中圈走 PickAt。
	[[nodiscard]] const IModuleNode* PickUltraInfo(DirectX::XMFLOAT2 worldPos) const noexcept;

private:
	static constexpr std::size_t kRefineSlotCount_ = 3;
	static constexpr std::size_t kRefineButtonCount_ = 4;
	static constexpr float kRefineSlotLabelFont_ = 20.0f;

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

	// ---- 各块。顺序是买卖、炼成、奥义。炼成还没拆出来。----
	// 按这个顺序交出各块，供布局、画面和每帧刷新共用。
	[[nodiscard]] std::array<ModuleShopBlock*, 2> Blocks_() noexcept;
	// 按当前外壳把世界矩形和商店原点写给每一块。
	void LayoutBlocks_() noexcept;
	// 先布局，再让每一块重画并对齐。
	void SyncBlocks_() noexcept;

	// 买卖区。货架、货卡、锁、刷新和顶栏。
	ModuleShopTrade trade_;
	// 奥义区。锻造、翻页和内部栏。
	ModuleShopUltra ultra_;
};

// 这颗 Node 现在归谁。奥义和店有炼成必须先于场/仓 FindNodeIndex，否则会认错主人。
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
			shop != nullptr && (shop->IsShopUltraOwned(node) || shop->IsShopRefineOwned(node)))
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