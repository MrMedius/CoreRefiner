#include "Graphics.h"
#include "RenderGraph.h"

#include "GameStatsCodex.h"

#include "ModuleShop.h"
#include "ModuleShopDraw.h"

#include <algorithm>
#include <utility>

using namespace ModuleShopDraw;

// ---- 外壳 ----

ModuleShop::ModuleShop()
{
	// 找主人、取出 Node、结果格归属都问商店自己。
	refine_.SetHostZone(this);
}

ModuleShop::BoundsWorld ModuleShop::GetShellBoundsWorld() const noexcept
{
	BoundsWorld b{};
	b.half = shellHalf_;
	b.center = DirectX::XMFLOAT2{
		origin_.x,
		origin_.y
	};
	return b;
}

void ModuleShop::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	origin_ = origin;
	// 先把新原点写给三块，再把结果格和货架 Node 按新原点摆正。
	SyncBlocks_();
	refine_.RealignResult();
	trade_.SyncNodes();
}

void ModuleShop::SetShellExtent(float halfX, float halfY) noexcept
{
	shellHalf_.x = (std::max)(1.0f, halfX);
	shellHalf_.y = (std::max)(1.0f, halfY);
	LayoutBlocks_();
	if (gfx_ != nullptr && rg_ != nullptr)
	{
		for (ModuleShopBlock* block : Blocks_())
		{
			block->Ensure(*gfx_, *rg_);
		}
	}
	// 框变宽后货卡间距变了，商品要按新格子重摆。结果格按新栏位重摆。
	trade_.Relayout();
	refine_.RealignResult();
	SyncBlocks_();
}

ModuleShop::BoundsWorld ModuleShop::GetTradeBoundsWorld() const noexcept
{
	const BoundsWorld inner = ShellInnerRect_();
	const float tradeH = SplitUnitHeight_() * static_cast<float>(kTradeRatio);
	BoundsWorld b{};
	b.half = DirectX::XMFLOAT2{
		ContentHalfX_(),
		tradeH * 0.5f
	};
	const float innerTop = inner.center.y - inner.half.y;
	b.center = DirectX::XMFLOAT2{
		inner.center.x,
		innerTop + b.half.y
	};
	return b;
}

ModuleShop::BoundsWorld ModuleShop::GetBoundsWorld() const noexcept
{
	return GetTradeBoundsWorld();
}

bool ModuleShop::ContainsCircle(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept
{
	(void)radius;
	const BoundsWorld trade = GetTradeBoundsWorld();
	const BoundsWorld refine = FunctionBandBounds_(0);
	const BoundsWorld ultra = FunctionBandBounds_(1);
	return PointInAabb_(trade.center, trade.half, worldCenter)
		|| PointInAabb_(refine.center, refine.half, worldCenter)
		|| PointInAabb_(ultra.center, ultra.half, worldCenter);
}

ModuleShop::BoundsWorld ModuleShop::ShellInnerRect_() const noexcept
{
	const BoundsWorld shell = GetShellBoundsWorld();
	BoundsWorld inner{};
	inner.center = shell.center;
	inner.half = DirectX::XMFLOAT2{
		shell.half.x - kShellInnerPad,
		shell.half.y - kShellInnerPad
	};
	return inner;
}

float ModuleShop::SplitUnitHeight_() const noexcept
{
	const float innerH = shellHalf_.y * 2.0f - 2.0f * kShellInnerPad;
	const float ratioSum = static_cast<float>(kTradeRatio)
		+ static_cast<float>(kFunctionRatio) * static_cast<float>(kFunctionBandCount_);
	return (innerH - 2.0f * kShellInnerGap) / ratioSum;
}

float ModuleShop::ContentHalfX_() const noexcept
{
	return shellHalf_.x - kShellInnerPad;
}

ModuleShop::BoundsWorld ModuleShop::FunctionBandBounds_(std::size_t band) const noexcept
{
	const BoundsWorld inner = ShellInnerRect_();
	const float unit = SplitUnitHeight_();
	const float tradeH = unit * static_cast<float>(kTradeRatio);
	const float funcH = unit * static_cast<float>(kFunctionRatio);
	const float innerTop = inner.center.y - inner.half.y;
	const float top = innerTop + tradeH + kShellInnerGap + static_cast<float>(band) * (funcH + kShellInnerGap);

	BoundsWorld b{};
	b.half = DirectX::XMFLOAT2{ ContentHalfX_(), funcH * 0.5f };
	b.center = DirectX::XMFLOAT2{ inner.center.x, top + b.half.y };
	return b;
}

// ---- 统一驱动 ----

void ModuleShop::RefreshCopy()
{
	trade_.RefreshCopy();
	refine_.RefreshCopy();
}

void ModuleShop::TickHud(float dt)
{
	// 先写入矩形和原点，再让各块走自己的每帧刷新。买卖区走倒计时和顶栏，炼成区在按钮或金额变化时重画，奥义区重画跟着钱变化的按钮和数字。
	LayoutBlocks_();
	for (ModuleShopBlock* block : Blocks_())
	{
		block->Tick(dt);
	}
}

void ModuleShop::InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	LayoutBlocks_();
	for (ModuleShopBlock* block : Blocks_())
	{
		block->Ensure(gfx, rg);
	}
	// 买卖区这时已经记下画面设备，补货时自己给商品建图。
	FillStock();
	SyncBlocks_();
}

void ModuleShop::SyncZoneTransforms_()
{
	SyncBlocks_();
	trade_.SyncNodes();
	// 炼成结果和店有的素材、主体按 hostOrigin_ 对齐。
	refine_.SyncNodes();
	// 奥义本体和看得见的内部 Node 由奥义区按 hostOrigin_ 对齐。
	ultra_.SyncNodes();
	SyncHud();
}

void ModuleShop::SubmitZoneBackground_()
{
	// 底板顺序是买卖、炼成、奥义。
	for (ModuleShopBlock* block : Blocks_())
	{
		block->Submit();
	}
}

std::array<ModuleShopBlock*, 3> ModuleShop::Blocks_() noexcept
{
	// 顺序是买卖、炼成、奥义。
	return { &trade_, &refine_, &ultra_ };
}

void ModuleShop::LayoutBlocks_() noexcept
{
	// 买卖区用外壳切出的买卖框。炼成区用上面那条功能带，奥义区用下面那条。
	trade_.SetBounds(GetTradeBoundsWorld());
	trade_.SetHostOrigin(origin_);
	refine_.SetBounds(FunctionBandBounds_(0));
	refine_.SetHostOrigin(origin_);
	ultra_.SetBounds(FunctionBandBounds_(1));
	ultra_.SetHostOrigin(origin_);
}

void ModuleShop::SyncBlocks_() noexcept
{
	LayoutBlocks_();
	for (ModuleShopBlock* block : Blocks_())
	{
		block->Sync();
	}
}

// ---- 跨块规则 ----

std::size_t ModuleShop::GetNodeCount() const noexcept
{
	// 货架长度跟 kSlotCount 走。炼成交出自己的固定段。奥义有几颗由奥义区自己算。
	return kSlotCount + refine_.GetZoneNodeCount() + ultra_.GetZoneNodeCount();
}

IModuleNode* ModuleShop::GetNode(std::size_t index) const noexcept
{
	if (index < kSlotCount)
	{
		return trade_.GetZoneNode(index);
	}
	const std::size_t refineIndex = index - kSlotCount;
	if (refineIndex < refine_.GetZoneNodeCount())
	{
		// 炼成区下标从 0 另计：0 是结果，1 和 2 是店有的素材、主体。
		return refine_.GetZoneNode(refineIndex);
	}
	// 奥义区下标从 0 另计：0 是本体，后面是看得见且不为空的内部 Node。
	return ultra_.GetZoneNode(index - (kSlotCount + refine_.GetZoneNodeCount()));
}

bool ModuleShop::TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos)
{
	if (node == nullptr)
	{
		return false;
	}
	const ModuleShopUltra::DropClaim claim = ultra_.TryAcceptDrop(node, localPos);
	if (claim != ModuleShopUltra::DropClaim::Ignore)
	{
		return claim == ModuleShopUltra::DropClaim::Accept;
	}
	if (node->IsCore())
	{
		return false;
	}

	// 买卖框只卖玩家的 Node。奥义本体也卖，里面的 Node 一起销毁。
	GameStatsCodex::AddCurrency(node->GetSellPrice());
	node.reset();
	return true;
}

std::unique_ptr<IModuleNode> ModuleShop::TakeNode(IModuleNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	if (std::unique_ptr<IModuleNode> taken = ultra_.TakeNode(node))
	{
		return taken;
	}
	if (std::unique_ptr<IModuleNode> taken = refine_.TakeNode(node))
	{
		return taken;
	}
	return trade_.TakeNode(node);
}

void ModuleShop::SubmitNodes()
{
	trade_.SubmitNodes();
	refine_.SubmitNodes();
	// 奥义本体和看得见的内部 Node 由奥义区提交。
	ultra_.SubmitNodes();
}

IModuleNode* ModuleShop::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	float refineDistSq = 1.0e9f;
	if (IModuleNode* refineNode = refine_.PickAt(worldPos, refineDistSq))
	{
		bestDistSq = refineDistSq;
		best = refineNode;
	}

	float shelfDistSq = 1.0e9f;
	if (IModuleNode* shelfNode = trade_.PickAt(worldPos, shelfDistSq))
	{
		if (shelfDistSq < bestDistSq)
		{
			bestDistSq = shelfDistSq;
			best = shelfNode;
		}
	}

	float ultraDistSq = 1.0e9f;
	if (IModuleNode* ultraNode = ultra_.PickAt(worldPos, ultraDistSq))
	{
		if (ultraDistSq < bestDistSq)
		{
			bestDistSq = ultraDistSq;
			best = ultraNode;
		}
	}

	if (best != nullptr)
	{
		outDistSq = bestDistSq;
	}
	return best;
}

DropResult ModuleShop::EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept
{
	DropResult result{};
	if (!ContainsCircle(worldPos, node.GetHitRadius()))
	{
		result.verdict = DropVerdict::OutOfBounds;
		return result;
	}

	const bool shopOwned = IsShopRefineOwned(&node);
	const std::size_t refineHit = refine_.HitParkSlot(worldPos);
	if (refineHit < ModuleShopRefine::kParkCount)
	{
		if (!CanParkRefine(node, refineHit))
		{
			result.verdict = DropVerdict::Forbidden;
			return result;
		}
		result.verdict = DropVerdict::Accept;
		const DirectX::XMFLOAT2 wc = refine_.SlotWorldCenter(refineHit);
		result.localPos = DirectX::XMFLOAT2{ wc.x - origin_.x, wc.y - origin_.y };
		return result;
	}

	const BoundsWorld refine = FunctionBandBounds_(0);
	if (PointInAabb_(refine.center, refine.half, worldPos))
	{
		result.verdict = DropVerdict::Forbidden;
		return result;
	}

	const BoundsWorld ultra = FunctionBandBounds_(1);
	if (PointInAabb_(ultra.center, ultra.half, worldPos))
	{
		// 奥义区的落点由奥义区自己定。这里拦住，避免掉进货架或卖掉。
		DropResult ultraDrop = ultra_.EvalDrop(node, worldPos);
		if (trade_.FindSlotIndex(&node) < kSlotCount)
		{
			// 货架只买进已有奥义的空内部栏。钱不够用红圈。
			const bool canBuy = IsDropAccepted(ultraDrop.verdict)
				&& ultra_.HasUltra()
				&& !node.IsCore()
				&& node.GetKind() != ModuleNodeKind::Ultra;
			if (!canBuy)
			{
				if (IsDropAccepted(ultraDrop.verdict))
				{
					ultraDrop.verdict = DropVerdict::Forbidden;
				}
				return ultraDrop;
			}
			if (GameStatsCodex::GetCurrency() < node.GetBuyPrice())
			{
				ultraDrop.verdict = DropVerdict::Unaffordable;
			}
		}
		return ultraDrop;
	}
	if (IsShopUltraOwned(&node))
	{
		// 奥义区拖到买卖框：卖掉。不写 localPos，避免停在商店原点或进货架。
		result.verdict = DropVerdict::Accept;
		return result;
	}
	if (node.GetKind() == ModuleNodeKind::Ultra)
	{
		// 场上或仓库的奥义拖到买卖框：卖掉，不放进货架。
		result.verdict = DropVerdict::Accept;
		return result;
	}

	if (shopOwned)
	{
		// 店有炼成 Node：买卖框非 Core 卖掉。场/仓不能丢进结果格（上面已拒）。
		if (node.IsCore())
		{
			result.verdict = DropVerdict::Forbidden;
			return result;
		}
		result.verdict = DropVerdict::Accept;
		return result;
	}

	if (from == ZoneId::Shop)
	{
		result.verdict = DropVerdict::Accept;
		const std::size_t i = trade_.FindSlotIndex(&node);
		if (i < kSlotCount)
		{
			result.localPos = trade_.SlotLocalPos(i);
		}
		return result;
	}

	if (node.IsCore())
	{
		result.verdict = DropVerdict::Forbidden;
		return result;
	}

	if (from == ZoneId::Field || from == ZoneId::Warehouse)
	{
		result.verdict = DropVerdict::Accept;
		return result;
	}

	result.verdict = DropVerdict::Forbidden;
	return result;
}

bool ModuleShop::CanParkRefine(const IModuleNode& node, std::size_t slot) const noexcept
{
	// 货架上的商品不能停进炼成。其余交给炼成区。
	if (trade_.Owns(&node))
	{
		return false;
	}
	return refine_.CanPark(node, slot);
}

void ModuleShop::ParkRefine(std::size_t slot, IModuleNode& node, IModuleZone* owner)
{
	refine_.Park(slot, node, owner);
	// 被挤走的奥义内部 Node 要回到自己的格子。
	SyncUltraAfterRefine_();
}

void ModuleShop::ClearRefineParks()
{
	refine_.ClearParks();
	// 退回的奥义内部 Node 要回到自己的格子。
	SyncUltraAfterRefine_();
}

bool ModuleShop::TryClickRefine(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse)
{
	const bool clicked = refine_.TryClick(worldPos, field, warehouse);
	// 升级、合成、进化或退回可能取走奥义内部 Node。
	SyncUltraAfterRefine_();
	return clicked;
}

void ModuleShop::ClearUltra()
{
	ForgetUltraNodesInRefine_();
	ultra_.ClearForRunEnd();
}

bool ModuleShop::TryPurchaseShelfIntoUltra(IModuleNode* node, DirectX::XMFLOAT2 localPos)
{
	const std::size_t index = trade_.FindSlotIndex(node);
	if (node == nullptr || index >= kSlotCount)
	{
		return false;
	}
	const int price = node->GetBuyPrice();
	std::unique_ptr<IModuleNode> taken = TakeNode(node);
	if (taken == nullptr)
	{
		return false;
	}
	if (!TryAcceptUltraDrop(taken, localPos))
	{
		RestoreShelfNode(std::move(taken), index);
		return false;
	}
	if (!GameStatsCodex::TrySpendCurrency(price))
	{
		std::unique_ptr<IModuleNode> back = TakeNode(node);
		if (back != nullptr)
		{
			RestoreShelfNode(std::move(back), index);
		}
		return false;
	}
	MarkSold(index);
	return true;
}

void ModuleShop::SellFromUltra(IModuleNode* node)
{
	if (node == nullptr || !IsShopUltraOwned(node))
	{
		return;
	}
	const int price = node->GetSellPrice();
	if (IsUltraBody(node))
	{
		ForgetUltraNodesInRefine_();
		ultra_.ClearForRunEnd();
	}
	else
	{
		UnbindRefine(node);
		std::unique_ptr<IModuleNode> taken = TakeNode(node);
		if (taken == nullptr)
		{
			return;
		}
		taken.reset();
	}
	GameStatsCodex::AddCurrency(price);
}

bool ModuleShop::SellsOnTradeDrop(const IModuleNode& node) const noexcept
{
	if (node.IsCore() || IsShelfStock(&node))
	{
		return false;
	}
	return true;
}

void ModuleShop::SyncUltraAfterRefine_()
{
	ultra_.Sync();
}

void ModuleShop::ForgetUltraNodesInRefine_() noexcept
{
	for (std::size_t i = 0; i < ModuleShopRefine::kParkCount; ++i)
	{
		IModuleNode* parked = refine_.GetParked(i);
		if (parked != nullptr && ultra_.Owns(parked))
		{
			refine_.ForgetParked(parked);
		}
	}
}

// ---- 转发 · 买卖 ----

void ModuleShop::FillStock()
{
	trade_.FillStock();
}

void ModuleShop::BeginVisit()
{
	// 同一局再次打开商店只刷新货架，奥义区保留。
	trade_.BeginVisit();
}

bool ModuleShop::TryRefresh()
{
	return trade_.TryRefresh();
}

void ModuleShop::ResetVisit()
{
	trade_.ResetVisit();
}

void ModuleShop::SyncHud()
{
	trade_.SyncHud();
}

void ModuleShop::SubmitHud()
{
	trade_.SubmitOverlay();
}

bool ModuleShop::HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return trade_.HitRefreshButton(worldPos);
}

bool ModuleShop::HitLockButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return trade_.HitLockButton(worldPos);
}

bool ModuleShop::ToggleLockAt(DirectX::XMFLOAT2 worldPos)
{
	return trade_.ToggleLockAt(worldPos);
}

int ModuleShop::GetRefreshCost() const noexcept
{
	return trade_.GetRefreshCost();
}

void ModuleShop::MarkSold(std::size_t index)
{
	trade_.MarkSold(index);
}

void ModuleShop::RestoreShelfNode(std::unique_ptr<IModuleNode> node, std::size_t index)
{
	trade_.RestoreShelfNode(std::move(node), index);
}

bool ModuleShop::IsShelfStock(const IModuleNode* node) const noexcept
{
	return trade_.Owns(node);
}

// ---- 转发 · 炼成 ----

std::size_t ModuleShop::HitRefineParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return refine_.HitParkSlot(worldPos);
}

DirectX::XMFLOAT2 ModuleShop::RefineSlotWorldCenter(std::size_t slot) const noexcept
{
	return refine_.SlotWorldCenter(slot);
}

void ModuleShop::UnbindRefine(IModuleNode* node) noexcept
{
	refine_.Unbind(node);
}

void ModuleShop::ClearRefineResult()
{
	refine_.ClearResult();
}

void ModuleShop::RestoreRefineResult(std::unique_ptr<IModuleNode> node)
{
	refine_.RestoreResult(std::move(node));
}

void ModuleShop::RestoreRefineHeld(std::unique_ptr<IModuleNode> node, std::size_t slot)
{
	refine_.RestoreHeld(std::move(node), slot);
}

std::size_t ModuleShop::FindRefineParkIndex(const IModuleNode* node) const noexcept
{
	return refine_.FindParkIndex(node);
}

bool ModuleShop::IsRefineParked(const IModuleNode* node) const noexcept
{
	return refine_.IsParked(node);
}

bool ModuleShop::IsRefineResultParked(const IModuleNode* node) const noexcept
{
	return refine_.IsResultParked(node);
}

bool ModuleShop::IsShopRefineOwned(const IModuleNode* node) const noexcept
{
	return refine_.Owns(node);
}

void ModuleShop::ApplyRefineParkIcon(IModuleNode& node) noexcept
{
	refine_.ApplyParkIcon(node);
}

void ModuleShop::PinShopRefineHomeGhost(IModuleNode& node) noexcept
{
	refine_.PinHomeGhost(node);
}

bool ModuleShop::HitRefineButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return refine_.HitButton(worldPos);
}

// ---- 转发 · 奥义 ----

ModuleShopUltra& ModuleShop::GetUltraArea() noexcept
{
	return ultra_;
}

const ModuleShopUltra& ModuleShop::GetUltraArea() const noexcept
{
	return ultra_;
}

bool ModuleShop::IsShopUltraOwned(const IModuleNode* node) const noexcept
{
	return ultra_.Owns(node);
}

bool ModuleShop::IsUltraBody(const IModuleNode* node) const noexcept
{
	return node != nullptr && ultra_.GetUltra() == node;
}

void ModuleShop::PinUltraHomeGhost(IModuleNode& node) noexcept
{
	ultra_.PinHomeGhost(node);
}

void ModuleShop::RestoreUltra(std::unique_ptr<IModuleNode> node)
{
	ultra_.Restore(std::move(node));
}

std::size_t ModuleShop::FindUltraInnerIndex(const IModuleNode* node) const noexcept
{
	return ultra_.FindInnerIndex(node);
}

void ModuleShop::RestoreUltraInner(std::unique_ptr<IModuleNode> node, std::size_t index)
{
	ultra_.RestoreInner(std::move(node), index);
}

bool ModuleShop::TryAcceptUltraDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos)
{
	if (node == nullptr)
	{
		return false;
	}
	return ultra_.TryAcceptDrop(node, localPos) == ModuleShopUltra::DropClaim::Accept;
}

bool ModuleShop::HitUltraArea(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const BoundsWorld ultra = FunctionBandBounds_(1);
	return PointInAabb_(ultra.center, ultra.half, worldPos);
}

bool ModuleShop::HitUltraButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return ultra_.HitButton(worldPos) != ModuleShopUltra::Button::None;
}

bool ModuleShop::TryClickUltra(DirectX::XMFLOAT2 worldPos)
{
	return ultra_.TryClick(worldPos);
}

bool ModuleShop::TakeUltraRenameRequest() noexcept
{
	return ultra_.TakeRenameRequest();
}

bool ModuleShop::TakeUltraRedrawRequest() noexcept
{
	return ultra_.TakeRedrawRequest();
}

const IModuleNode* ModuleShop::PickUltraInfo(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return ultra_.PickInfoNode(worldPos);
}

