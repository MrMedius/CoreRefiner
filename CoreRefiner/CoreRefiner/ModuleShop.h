#pragma once
#include "IModuleNode.h"
#include "IModuleZone.h"
#include "ModuleShopBlock.h"
#include "ModuleShopRefine.h"
#include "ModuleShopTrade.h"
#include "ModuleShopUltra.h"

#include <array>
#include <cstddef>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

// 商店是唯一的 ZoneId::Shop。外壳、三块的统一驱动、跨块规则和转发都在这里。
class ModuleShop : public IModuleZone
{
public:
	ModuleShop();
	~ModuleShop() override = default;

	ModuleShop(const ModuleShop&) = delete;
	ModuleShop& operator=(const ModuleShop&) = delete;

	// ---- 外壳：整店外框，以及 2:1:1 竖切出的买卖 / 炼成 / 奥义 ----
	// 默认外壳半宽、半高。还没被编辑器改过时用这一对。
	static constexpr float kFrozenShellHalfX = 332.0f;
	static constexpr float kFrozenShellHalfY = 230.0f;
	// 三块内容板离外壳边的空隙。
	static constexpr float kShellInnerPad = 12.0f;
	// 买卖框和两条功能带之间的空隙。
	static constexpr float kShellInnerGap = 12.0f;
	// 竖切比例。买卖占 2 份，炼成和奥义各占 1 份。
	static constexpr int kTradeRatio = 2;
	static constexpr int kFunctionRatio = 1;
	// 功能带条数。0 是炼成，1 是奥义。
	static constexpr std::size_t kFunctionBandCount_ = 2;

	// 区号。商店固定是 Shop。
	[[nodiscard]] ZoneId GetZoneId() const noexcept override { return ZoneId::Shop; }
	// 商店原点。三块用它把世界坐标换成 Node 的 localPos。
	[[nodiscard]] DirectX::XMFLOAT3 GetOrigin() const noexcept override { return origin_; }
	// 记下新原点，写给三块，再把结果格和货架按新原点摆正。
	void SetOrigin(DirectX::XMFLOAT3 origin) noexcept override;
	// 改外壳半宽半高。画面已建好时让三块重建画布，并按新格子重摆货架和结果格。
	void SetShellExtent(float halfX, float halfY) noexcept;
	// 整店外框的世界矩形。中心在原点，半宽半高是 shellHalf_。
	[[nodiscard]] BoundsWorld GetShellBoundsWorld() const noexcept override;
	// 买卖框的世界矩形。按 2:1:1 从外壳切出来，交给 trade_ 当 bounds。
	[[nodiscard]] BoundsWorld GetTradeBoundsWorld() const noexcept;
	// 区接口要的代表矩形。商店用买卖框。
	[[nodiscard]] BoundsWorld GetBoundsWorld() const noexcept override;
	// 点是否落在买卖、炼成、奥义三块矩形之一里。半径不参与判断。
	[[nodiscard]] bool ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept override;

private:
	// 外壳去掉内边距后的矩形。三块都排在这里面。
	[[nodiscard]] BoundsWorld ShellInnerRect_() const noexcept;
	// 2:1:1 里每一份的高度。空隙已经扣掉。
	[[nodiscard]] float SplitUnitHeight_() const noexcept;
	// 三块内容板共用的半宽，和买卖框相同。
	[[nodiscard]] float ContentHalfX_() const noexcept;
	// 第 band 条功能带的世界矩形。0 是炼成，1 是奥义。
	[[nodiscard]] BoundsWorld FunctionBandBounds_(std::size_t band) const noexcept;

	// 商店原点。
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 外壳半宽半高。默认是冻结尺寸。
	DirectX::XMFLOAT2 shellHalf_{ kFrozenShellHalfX, kFrozenShellHalfY };
	// 画面设备。InitZoneVisuals_ 之后才有。
	Graphics* gfx_{ nullptr };
	// 渲染图。和 gfx_ 一起记下。
	Rgph::RenderGraph* rg_{ nullptr };

public:
	// ---- 统一驱动：把矩形、画面和每帧刷新交给买卖、炼成、奥义 ----
	// 换语言：买卖区重画货卡，炼成区重画底板。
	void RefreshCopy();
	// 每帧把矩形和原点写给三块，再让各块自己刷新。
	void TickHud(float dt);

protected:
	// 记下画面设备，让三块按当前矩形建画布，再给货架补货。
	void InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg) override;
	// 对齐三块底板，再对齐货架、炼成和奥义各自的 Node。
	void SyncZoneTransforms_() override;
	// 按买卖、炼成、奥义提交底板。画在 Node 下面。
	void SubmitZoneBackground_() override;

private:
	// 按买卖、炼成、奥义交出三块，供布局、画面和每帧刷新共用。
	[[nodiscard]] std::array<ModuleShopBlock*, 3> Blocks_() noexcept;
	// 按当前外壳把世界矩形和商店原点写给每一块。
	void LayoutBlocks_() noexcept;
	// 先布局，再让每一块重画并对齐。
	void SyncBlocks_() noexcept;

	// 买卖区。货架、货卡、锁、刷新和顶栏。
	ModuleShopTrade trade_;
	// 炼成区。三格停放、四个操作和底板。
	ModuleShopRefine refine_;
	// 奥义区。锻造、翻页和内部栏。
	ModuleShopUltra ultra_;

public:
	// ---- 跨块规则：下标、拾取、拖放，以及一块的结果要通知另一块的地方 ----
	// 货架列数和格数。和买卖区是同一份，编辑器靠 kSlotCount 认出货架下标。
	static constexpr int kColumns = ModuleShopTrade::kColumns;
	static constexpr std::size_t kSlotCount = ModuleShopTrade::kSlotCount;

	// [0, kSlotCount) 货架。接着炼成区交出结果和两颗店有 Node。再往后由奥义区按自己的下标交出。
	[[nodiscard]] std::size_t GetNodeCount() const noexcept override;
	[[nodiscard]] IModuleNode* GetNode(std::size_t index) const noexcept override;
	// 松手时先问奥义区。对不上就按卖出处理，核心拒绝。
	[[nodiscard]] bool TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos) override;
	// 取出顺序是奥义、店有炼成、货架。不是这三处的返回空。
	[[nodiscard]] std::unique_ptr<IModuleNode> TakeNode(IModuleNode* node) override;
	// 按买卖、炼成、奥义提交要画出来的 Node。
	void SubmitNodes() override;
	// 点选顺序是炼成停放、货架、奥义。距离更近的才替换，相等不算。
	[[nodiscard]] IModuleNode* PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept override;
	// 落点先问炼成格，再拦炼成带和奥义带，最后才是买卖框的卖出或放回货架。
	[[nodiscard]] DropResult EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept override;

	// 货架商品先拒绝，其余问炼成区能不能停。
	[[nodiscard]] bool CanParkRefine(const IModuleNode& node, std::size_t slot) const noexcept;
	// 把 Node 停进这一格。owner 用来把世界中心写成 localPos。调用后重摆奥义区。
	void ParkRefine(std::size_t slot, IModuleNode& node, IModuleZone* owner);
	// 素材和主体退回原处。调用后重摆奥义区。
	void ClearRefineParks();
	// 点到升级、合成、进化或退回就执行。调用后重摆奥义区。
	bool TryClickRefine(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse);
	// 结束本局时先摘掉炼成里指向奥义的停放，再清空奥义区。再次打开商店不调用它。
	void ClearUltra();
	// 从货架买进奥义内部栏。失败时商品回到原货槽。
	bool TryPurchaseShelfIntoUltra(IModuleNode* node, DirectX::XMFLOAT2 localPos);
	// 从奥义区卖到买卖框。卖掉本体前先摘掉炼成里的停放，本体连同内部 Node 一起销毁。
	void SellFromUltra(IModuleNode* node);
	// 商店悬停弹 Panel：炼成停放、奥义本体、内部栏。货架不弹。
	[[nodiscard]] bool ShowsHoverInfo(const IModuleNode* node) const noexcept
	{
		return IsRefineParked(node) || IsShopUltraOwned(node);
	}
	// 拖到买卖框时会卖掉。货架上的商品和核心不算。
	[[nodiscard]] bool SellsOnTradeDrop(const IModuleNode& node) const noexcept;

private:
	// 炼成停放、点击或清空停放之后，把奥义内部 Node 摆回当前格子。
	void SyncUltraAfterRefine_();
	// 奥义即将被清掉时，摘掉炼成格里指向这些 Node 的停放。只清指针，不重画。
	void ForgetUltraNodesInRefine_() noexcept;

public:
	// ---- 转发 · 买卖 ----
	// 按可刷表给货架补货。画面已建好时，买卖区自己给商品建图。
	void FillStock();
	// 清掉本局刷新次数，再重抽未锁定的货。同一局再次打开只走这里。
	void BeginVisit();
	// 钱够且还有可重抽的货时，交给买卖区扣费并重抽。
	[[nodiscard]] bool TryRefresh();
	// 刷新次数和拒绝倒计时归零。
	void ResetVisit();
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
	// 还在货架上的商品。
	[[nodiscard]] bool IsShelfStock(const IModuleNode* node) const noexcept;

	// ---- 转发 · 炼成 ----
	// 炼成停放：0 素材，1 主体，2 结果。没命中返回炼成区的停放格个数。
	[[nodiscard]] std::size_t HitRefineParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 第 slot 格中心的世界坐标。
	[[nodiscard]] DirectX::XMFLOAT2 RefineSlotWorldCenter(std::size_t slot) const noexcept;
	// 从停放格摘掉这颗 Node，并让炼成区重画。
	void UnbindRefine(IModuleNode* node) noexcept;
	// 丢掉结果和两颗店有 Node，并重画炼成底板。
	void ClearRefineResult();
	// 拖出失败时把 Node 放回结果格。
	void RestoreRefineResult(std::unique_ptr<IModuleNode> node);
	// 拖出失败时放回这一格。0 和 1 是店有素材、主体，其余是结果。
	void RestoreRefineHeld(std::unique_ptr<IModuleNode> node, std::size_t slot);
	// 这颗 Node 在第几格。不在炼成区时返回停放格个数。
	[[nodiscard]] std::size_t FindRefineParkIndex(const IModuleNode* node) const noexcept;
	// 三格停放指针里有没有这颗 Node。场、仓、奥义停进来的也算。
	[[nodiscard]] bool IsRefineParked(const IModuleNode* node) const noexcept;
	// 这颗 Node 是不是结果格里的那一颗。
	[[nodiscard]] bool IsRefineResultParked(const IModuleNode* node) const noexcept;
	// 结果或两颗店有 Node 的实体在炼成区手里。停进来的不算。
	[[nodiscard]] bool IsShopRefineOwned(const IModuleNode* node) const noexcept;
	// 格内图标按当前格子边长缩放。素材和主体的残影仍走来源尺寸。
	void ApplyRefineParkIcon(IModuleNode& node) noexcept;
	// 店有 Node 的残影钉在结果格。跟手图标由调用方清掉。
	void PinShopRefineHomeGhost(IModuleNode& node) noexcept;
	// 点是否落在炼成区右侧四个按钮之一上。
	[[nodiscard]] bool HitRefineButton(DirectX::XMFLOAT2 worldPos) const noexcept;

	// ---- 转发 · 奥义 ----
	// 奥义区本身。编辑器拿它问栏位和容量。
	[[nodiscard]] ModuleShopUltra& GetUltraArea() noexcept;
	[[nodiscard]] const ModuleShopUltra& GetUltraArea() const noexcept;
	// 奥义本体和内部栏里的 Node，含滚出画面的。
	[[nodiscard]] bool IsShopUltraOwned(const IModuleNode* node) const noexcept;
	// 只认奥义栏里的那一颗，不含内部栏。
	[[nodiscard]] bool IsUltraBody(const IModuleNode* node) const noexcept;
	// 残影留在奥义栏。跟手图标由调用方清掉。
	void PinUltraHomeGhost(IModuleNode& node) noexcept;
	// 拖出失败时放回奥义栏。
	void RestoreUltra(std::unique_ptr<IModuleNode> node);
	// 内部栏下标。不在里面时返回 npos。
	[[nodiscard]] std::size_t FindUltraInnerIndex(const IModuleNode* node) const noexcept;
	// 拖出失败时放回这个内部栏。
	void RestoreUltraInner(std::unique_ptr<IModuleNode> node, std::size_t index);
	// 只接奥义区落点。对不上就原样退回，不进货架、不卖掉。
	bool TryAcceptUltraDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos);
	// 点是否落在奥义区整块矩形里。
	[[nodiscard]] bool HitUltraArea(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 奥义区按钮。编辑器只问有没有点到，锻造、打孔和翻页由奥义区自己分。
	[[nodiscard]] bool HitUltraButton(DirectX::XMFLOAT2 worldPos) const noexcept;
	// 点到锻造、打孔、更名、重绘或翻页就执行。
	bool TryClickUltra(DirectX::XMFLOAT2 worldPos);
	// 这次点的是更名时返回 true。调用后记号消失。
	bool TakeUltraRenameRequest() noexcept;
	// 这次点的是重绘时返回 true。调用后记号消失。
	bool TakeUltraRedrawRequest() noexcept;
	// 鼠标在奥义图标上时返回它，给 Panel 用。选中圈走 PickAt。
	[[nodiscard]] const IModuleNode* PickUltraInfo(DirectX::XMFLOAT2 worldPos) const noexcept;
};

// ---- 跨块规则 · 主人 ----
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
