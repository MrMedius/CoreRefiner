#pragma once
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleShopBlock.h"
#include "ModuleShopRefine.h"
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
	ModuleShop();
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

	// [0, kSlotCount) 货架。接着炼成区交出结果和两颗店有 Node。再往后由奥义区按自己的下标交出。
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
	// 换语言：买卖区重画货卡，炼成区重画底板。
	void RefreshCopy();
	// 每帧把矩形和原点写给各块，再让各块自己刷新。炼成按钮亮灭在炼成区里算。
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

	// ---- 炼成区。停放、按钮和店有 Node 在 refine_，这里只转发。货架拒绝留在商店。----
public:
	// 炼成停放：0 素材，1 主体，2 结果。没命中返回炼成区的停放格个数。
	[[nodiscard]] std::size_t HitRefineParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 第 slot 格中心的世界坐标。
	[[nodiscard]] DirectX::XMFLOAT2 RefineSlotWorldCenter(std::size_t slot) const noexcept;
	// 货架商品先拒绝，其余问炼成区能不能停。
	[[nodiscard]] bool CanParkRefine(const IModuleNode& node, std::size_t slot) const noexcept;
	// 把 Node 停进这一格。owner 用来把世界中心写成 localPos。调用后重摆奥义区。
	void ParkRefine(std::size_t slot, IModuleNode& node, IModuleZone* owner);
	// 从停放格摘掉这颗 Node，并让炼成区重画。
	void UnbindRefine(IModuleNode* node) noexcept;
	// 素材和主体退回原处。调用后重摆奥义区。
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
	// 点是否落在炼成区右侧四个按钮之一上。
	[[nodiscard]] bool HitRefineButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点到升级、合成、进化或退回就执行。调用后重摆奥义区。
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
	// ---- 各块。顺序是买卖、炼成、奥义。----
	// 按这个顺序交出各块，供布局、画面和每帧刷新共用。
	[[nodiscard]] std::array<ModuleShopBlock*, 3> Blocks_() noexcept;
	// 按当前外壳把世界矩形和商店原点写给每一块。
	void LayoutBlocks_() noexcept;
	// 先布局，再让每一块重画并对齐。
	void SyncBlocks_() noexcept;
	// 炼成停放、点击或清空停放之后，把奥义内部 Node 摆回当前格子。
	void SyncUltraAfterRefine_();
	// 奥义即将被清掉时，摘掉炼成格里指向这些 Node 的停放。只清指针，不重画。
	void ForgetUltraNodesInRefine_() noexcept;

	// 买卖区。货架、货卡、锁、刷新和顶栏。
	ModuleShopTrade trade_;
	// 炼成区。三格停放、四个操作和底板。
	ModuleShopRefine refine_;
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