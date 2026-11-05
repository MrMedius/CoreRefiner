#include "ZoneLayoutEditor.h"
#include "ModuleShop.h"
#include "GameRulesCodex.h"
#include "GameStatsCodex.h"
#include "InputCodex.h"
#include "Colors.h"
#include "Channels.h"
#include "Graphics.h"
#include "Window.h"

#include <cmath>
#include <utility>

namespace
{
	[[nodiscard]] bool IsBoundZoneId_(ZoneId id) noexcept
	{
		return ToIndex(id) < ZoneCount();
	}
}

void ZoneLayoutEditor::Begin(std::array<IModuleZone*, ZoneCount()> zones, std::array<DirectX::XMFLOAT3, ZoneCount()> origins, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 combatFieldOrigin)
{
	combatOrigin_ = combatFieldOrigin;
	zones_ = zones;

	EnsureRingVisual_(gfx, rg);
	infoPanel_.Ensure(gfx, rg);
	infoPanel_.Hide();

	hover_ = nullptr;
	dragged_ = nullptr;
	hoverSource_ = kNoZone_;
	dragSource_ = kNoZone_;
	active_ = true;
	dragStartLocalPos_ = {};
	dragOrigin_ = {};
	ringKind_ = RingKind_::Hover;

	for (std::size_t i = 0; i < ZoneCount(); ++i)
	{
		IModuleZone* zone = zones_[i];
		if (zone == nullptr)
		{
			continue;
		}
		zone->SetOrigin(origins[i]);
		zone->SyncAllVisuals();
	}
}

void ZoneLayoutEditor::End()
{
	if (!active_)
	{
		return;
	}

	ClearActiveDrag_();
	if (auto* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop)))
	{
		shop->ClearRefineParks();
	}
	ClearAllLayoutGhosts_();
	infoPanel_.Hide();

	hover_ = nullptr;
	hoverSource_ = kNoZone_;
	ringKind_ = RingKind_::Hover;
	dragStartLocalPos_ = {};

	if (IModuleZone* field = ZoneAt_(ZoneId::Field))
	{
		field->SetOrigin(combatOrigin_);
		field->SyncAllVisuals();
	}

	zones_.fill(nullptr);
	active_ = false;
}

void ZoneLayoutEditor::Update(float dt, Window* hostWindow)
{
	if (!active_)
	{
		return;
	}

	auto& input = InputCodex::Get();
	ModuleShop* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop));
	if (shop != nullptr)
	{
		shop->TickHud(dt);
	}

	const DirectX::XMFLOAT2 mouseGame = MouseGame_();

	if (dragged_ != nullptr)
	{
		if (input.MouseLeftPressed())
		{
			SetFreePreview_(*dragged_, mouseGame);
			hover_ = dragged_;
			hoverSource_ = dragSource_;
		}
		if (input.MouseLeftReleased())
		{
			SetFreePreview_(*dragged_, mouseGame);
			ResolveRelease_();
			dragged_ = nullptr;
			dragSource_ = kNoZone_;
			dragOrigin_ = {};
			hover_ = PickHover_(mouseGame, hoverSource_);
		}
	}
	else if (shop != nullptr && shop->HitRefineButton(mouseGame))
	{
		hover_ = nullptr;
		hoverSource_ = kNoZone_;
		if (input.MouseLeftTriggered())
		{
			(void)shop->TryClickRefine(
				mouseGame,
				ZoneAt_(ZoneId::Field),
				ZoneAt_(ZoneId::Warehouse));
		}
	}
	else if (shop != nullptr && shop->HitRefreshButton(mouseGame))
	{
		hover_ = nullptr;
		hoverSource_ = kNoZone_;
		if (input.MouseLeftTriggered())
		{
			(void)shop->TryRefresh();
		}
	}
	else if (shop != nullptr && shop->HitLockButton(mouseGame))
	{
		hover_ = nullptr;
		hoverSource_ = kNoZone_;
		if (input.MouseLeftTriggered())
		{
			(void)shop->ToggleLockAt(mouseGame);
		}
	}
	else if (shop != nullptr && shop->HitUltraButton(mouseGame))
	{
		hover_ = nullptr;
		hoverSource_ = kNoZone_;
		if (input.MouseLeftTriggered())
		{
			(void)shop->TryClickUltra(mouseGame);
			// 商店只回答这次是不是更名。编辑器不分辨锻造、打孔和翻页。
			if (shop->TakeUltraRenameRequest())
			{
				RequestRename_();
			}
		}
	}
	else
	{
		hover_ = PickHover_(mouseGame, hoverSource_);
		if (input.MouseLeftTriggered() && hover_ != nullptr)
		{
			dragged_ = hover_;
			dragSource_ = hoverSource_;
			if (IModuleZone* owner = OwnerZoneOf_(dragged_))
			{
				dragOrigin_ = owner->GetOrigin();
			}
			else
			{
				dragOrigin_ = OriginForSource_(dragSource_);
			}
			dragStartLocalPos_ = dragged_->GetLocalPos();
			if (shop != nullptr && shop->IsShopRefineOwned(dragged_))
			{
				// 店有炼成 Node：残影钉在结果格（格内尺寸），跟手恢复 hitRadius。
				shop->PinShopRefineHomeGhost(*dragged_);
				dragged_->ClearIconRadiusOverride();
			}
			else if (shop != nullptr && shop->IsShopUltraOwned(dragged_) && !shop->IsRefineParked(dragged_))
			{
				// 第一次从奥义栏拿起才钉残影。已停在炼成格的从当前位置跟手。
				shop->PinUltraHomeGhost(*dragged_);
				dragged_->ClearIconRadiusOverride();
			}
			else if (!dragged_->IsLayoutGhostActive())
			{
				dragged_->BeginLayoutGhost(dragStartLocalPos_);
			}
			else
			{
				// 从素材/主体格拿起：Icon 改回跟手大小，残影仍留在来源。
				dragged_->ClearIconRadiusOverride();
			}
			if (hostWindow != nullptr)
			{
				SnapCursorToNode_(*dragged_, dragOrigin_, *hostWindow);
			}
			SetFreePreview_(*dragged_, DirectX::XMFLOAT2{
				dragOrigin_.x + dragged_->GetLocalPos().x,
				dragOrigin_.y + dragged_->GetLocalPos().y
			});
		}
	}

	if (shop != nullptr)
	{
		shop->SyncHud();
	}

	IModuleNode* ringTarget = (dragged_ != nullptr) ? dragged_ : hover_;
	if (dragged_ != nullptr)
	{
		const DropEval_ eval = EvalDrop_(*dragged_);
		if (eval.unaffordable)
		{
			ringKind_ = RingKind_::Denied;
		}
		else if (eval.placeable)
		{
			ringKind_ = RingKind_::Valid;
		}
		else
		{
			ringKind_ = RingKind_::Overlap;
		}
	}
	else
	{
		ringKind_ = RingKind_::Hover;
	}
	if (ringTarget != nullptr)
	{
		DirectX::XMFLOAT3 ringOrigin = dragOrigin_;
		if (dragged_ == nullptr)
		{
			if (IModuleZone* owner = OwnerZoneOf_(ringTarget))
			{
				ringOrigin = owner->GetOrigin();
			}
			else
			{
				ringOrigin = OriginForSource_(hoverSource_);
			}
		}
		SyncRingTransform_(*ringTarget, ringOrigin);
	}

	bool showInfo = dragged_ == nullptr && hover_ != nullptr;
	if (showInfo && hoverSource_ == ZoneId::Shop)
	{
		// 货架不弹。炼成停放和奥义区里的 Node 弹。
		showInfo = shop != nullptr && shop->ShowsHoverInfo(hover_);
	}
	if (!showInfo)
	{
		infoPanel_.Hide();
	}
	else
	{
		IModuleZone* owner = OwnerZoneOf_(hover_);
		const DirectX::XMFLOAT3 origin = (owner != nullptr)
			? owner->GetOrigin()
			: OriginForSource_(hoverSource_);
		const DirectX::XMFLOAT2 local = hover_->GetLocalPos();
		infoPanel_.ShowFor(
			*hover_,
			DirectX::XMFLOAT2{ origin.x + local.x, origin.y + local.y });
	}
}

void ZoneLayoutEditor::SubmitOverlay()
{
	if (!active_)
	{
		return;
	}
	if (dragged_ != nullptr || hover_ != nullptr)
	{
		if (Canvas2D* ring = ActiveRing_())
		{
			ring->Submit(Chan::ui);
		}
	}
	infoPanel_.Submit();
}

std::unique_ptr<Canvas2D> ZoneLayoutEditor::MakeRingCanvas_(Graphics& gfx, Rgph::RenderGraph& rg, Color ringColor)
{
	constexpr unsigned kSize = 64u;
	auto ring = std::make_unique<Canvas2D>(gfx, kSize, kSize);
	ring->Clear(Colors::None);

	const float cx = (static_cast<float>(kSize) - 1.0f) * 0.5f;
	const float cy = cx;
	const float rOuter = static_cast<float>(kSize) * 0.48f;
	const float rInner = rOuter - 3.5f;

	for (unsigned y = 0u; y < kSize; ++y)
	{
		for (unsigned x = 0u; x < kSize; ++x)
		{
			const float dx = static_cast<float>(x) - cx;
			const float dy = static_cast<float>(y) - cy;
			const float d = std::sqrt(dx * dx + dy * dy);
			if (d <= rOuter && d >= rInner)
			{
				ring->PutPixel(x, y, ringColor);
			}
		}
	}
	ring->NotifyPixelsChanged();
	ring->LinkTechniques(rg);
	return ring;
}

void ZoneLayoutEditor::EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (ringHover_ == nullptr)
	{
		ringHover_ = MakeRingCanvas_(gfx, rg, Color{ 255u, 230u, 80u, 230u });
	}
	if (ringValid_ == nullptr)
	{
		ringValid_ = MakeRingCanvas_(gfx, rg, Color{ 80u, 220u, 120u, 230u });
	}
	if (ringOverlap_ == nullptr)
	{
		ringOverlap_ = MakeRingCanvas_(gfx, rg, Color{ 230u, 80u, 80u, 230u });
	}
	if (ringDenied_ == nullptr)
	{
		ringDenied_ = MakeRingCanvas_(gfx, rg, Color{ 160u, 160u, 160u, 230u });
	}
}

void ZoneLayoutEditor::ClearAllLayoutGhosts_()
{
	for (IModuleZone* zone : zones_)
	{
		if (zone != nullptr)
		{
			zone->ClearLayoutGhosts();
		}
	}
}

DirectX::XMFLOAT2 ZoneLayoutEditor::MouseGame_() const noexcept
{
	const auto mouse = InputCodex::Get().MousePos();
	return DirectX::XMFLOAT2{
		static_cast<float>(mouse.first),
		static_cast<float>(mouse.second)
	};
}

DirectX::XMFLOAT3 ZoneLayoutEditor::OriginForSource_(ZoneId source) const noexcept
{
	if (IModuleZone* zone = ZoneAt_(source))
	{
		return zone->GetOrigin();
	}
	return DirectX::XMFLOAT3{};
}

DirectX::XMFLOAT2 ZoneLayoutEditor::WorldPosOf_(const IModuleNode& node, DirectX::XMFLOAT3 origin) const noexcept
{
	return DirectX::XMFLOAT2{
		origin.x + node.GetLocalPos().x,
		origin.y + node.GetLocalPos().y
	};
}

IModuleZone* ZoneLayoutEditor::ZoneAt_(ZoneId id) const noexcept
{
	if (!IsBoundZoneId_(id))
	{
		return nullptr;
	}
	return zones_[ToIndex(id)];
}

IModuleZone* ZoneLayoutEditor::OwnerZoneOf_(const IModuleNode* node) const noexcept
{
	return FindNodeOwnerZone(node, zones_.data(), ZoneCount());
}

void ZoneLayoutEditor::SetFreePreview_(IModuleNode& node, DirectX::XMFLOAT2 mouseGame)
{
	node.SetLocalPos(DirectX::XMFLOAT2{
		mouseGame.x - dragOrigin_.x,
		mouseGame.y - dragOrigin_.y
	});
	node.SyncVisual();
}

ZoneLayoutEditor::DropEval_ ZoneLayoutEditor::EvalDrop_(const IModuleNode& node) const noexcept
{
	DropEval_ eval{};
	const DirectX::XMFLOAT2 world = WorldPosOf_(node, dragOrigin_);
	const float radius = node.GetHitRadius();
	const ZoneId from = dragSource_;

	for (std::size_t i = 0; i < ZoneCount(); ++i)
	{
		IModuleZone* zone = zones_[i];
		if (zone == nullptr || !zone->ContainsCircle(world, radius))
		{
			continue;
		}
		const DropResult drop = zone->EvalDrop(node, world, from);
		if (drop.verdict == DropVerdict::Unaffordable)
		{
			eval.unaffordable = true;
			return eval;
		}
		if (!IsDropAccepted(drop.verdict))
		{
			return eval;
		}

		const ZoneId target = static_cast<ZoneId>(i);
		IModuleZone* owner = OwnerZoneOf_(&node);
		auto* ownerShop = dynamic_cast<ModuleShop*>(owner);
		const bool fromShopRefine = ownerShop != nullptr && ownerShop->IsShopRefineOwned(&node);
		const bool fromUltraOwned = ownerShop != nullptr && ownerShop->IsShopUltraOwned(&node);
		if (!fromShopRefine
			&& !fromUltraOwned
			&& owner != nullptr
			&& owner->GetZoneId() == ZoneId::Shop
			&& target != ZoneId::Shop)
		{
			const int price = node.GetBuyPrice();
			if (GameStatsCodex::GetCurrency() < price)
			{
				eval.unaffordable = true;
				eval.target = target;
				return eval;
			}
		}

		eval.placeable = true;
		eval.target = target;
		return eval;
	}
	return eval;
}

void ZoneLayoutEditor::RevertDrag_()
{
	if (dragged_ == nullptr)
	{
		return;
	}

	dragged_->SetLocalPos(dragStartLocalPos_);
	IModuleZone* owner = OwnerZoneOf_(dragged_);
	if (owner == nullptr)
	{
		owner = ZoneAt_(dragSource_);
	}
	if (owner != nullptr)
	{
		dragged_->SetZoneOrigin(owner->GetOrigin());
	}
	auto* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop));
	const bool keepGhost = shop != nullptr && shop->IsRefineParked(dragged_);
	if (!keepGhost)
	{
		dragged_->EndLayoutGhost();
	}
	else
	{
		shop->ApplyRefineParkIcon(*dragged_);
	}
	dragged_->SyncVisual();
	if (owner != nullptr)
	{
		owner->SyncAllVisuals();
	}
}

void ZoneLayoutEditor::ClearActiveDrag_()
{
	if (dragged_ == nullptr)
	{
		return;
	}
	RevertDrag_();
	dragged_ = nullptr;
	dragSource_ = kNoZone_;
	dragOrigin_ = {};
}

void ZoneLayoutEditor::SetOnOverlayRequest(std::function<void(const OverlayUiRequest&)> cb)
{
	onOverlay_ = std::move(cb);
}

void ZoneLayoutEditor::RequestRename_()
{
	if (!onOverlay_)
	{
		return;
	}
	auto* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop));
	if (shop == nullptr)
	{
		return;
	}
	IModuleNode* ultra = shop->GetUltraArea().GetUltra();
	if (ultra == nullptr)
	{
		return;
	}
	onOverlay_(OverlayUiRequest{ OverlayUiKind::Rename, ultra->GetInstanceId(), 0 });
}

bool ZoneLayoutEditor::RequestSellConfirm_(IModuleNode& node)
{
	const int price = node.GetSellPrice();
	// 卖价不超过确认线就直接卖。
	if (price <= GameRulesCodex::SellConfirmPrice)
	{
		return false;
	}

	const std::uint32_t instanceId = node.GetInstanceId();
	RevertDrag_();
	dragged_ = nullptr;
	dragSource_ = kNoZone_;
	dragOrigin_ = {};
	if (onOverlay_)
	{
		onOverlay_(OverlayUiRequest{ OverlayUiKind::SellConfirm, instanceId, price });
	}
	return true;
}

IModuleNode* ZoneLayoutEditor::FindByInstanceId_(std::uint32_t instanceId) const noexcept
{
	for (IModuleZone* zone : zones_)
	{
		if (zone == nullptr)
		{
			continue;
		}
		const std::size_t count = zone->GetNodeCount();
		for (std::size_t i = 0; i < count; ++i)
		{
			IModuleNode* node = zone->GetNode(i);
			if (node != nullptr && node->GetInstanceId() == instanceId)
			{
				return node;
			}
		}
	}

	// 滚出当前页的内部 Node 不在 GetNode 里，仍要能确认卖掉。
	auto* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop));
	if (shop == nullptr)
	{
		return nullptr;
	}
	ModuleShopUltra& ultra = shop->GetUltraArea();
	const int innerCount = ultra.GetInnerSlotCount();
	for (int i = 0; i < innerCount; ++i)
	{
		IModuleNode* node = ultra.GetInnerSlot(static_cast<std::size_t>(i));
		if (node != nullptr && node->GetInstanceId() == instanceId)
		{
			return node;
		}
	}
	return nullptr;
}

bool ZoneLayoutEditor::ConfirmSell(std::uint32_t instanceId)
{
	auto* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop));
	if (shop == nullptr)
	{
		return false;
	}
	IModuleNode* node = FindByInstanceId_(instanceId);
	if (node == nullptr)
	{
		return false;
	}
	if (shop->IsShopUltraOwned(node))
	{
		shop->SellFromUltra(node);
		return true;
	}
	if (shop->IsShelfStock(node))
	{
		return false;
	}

	shop->UnbindRefine(node);
	IModuleZone* owner = OwnerZoneOf_(node);
	if (owner == nullptr)
	{
		return false;
	}
	node->EndLayoutGhost();
	std::unique_ptr<IModuleNode> taken = owner->TakeNode(node);
	if (taken == nullptr)
	{
		return false;
	}
	GameStatsCodex::AddCurrency(taken->GetSellPrice());
	taken.reset();
	owner->SyncAllVisuals();
	return true;
}

void ZoneLayoutEditor::ClearPointerState()
{
	if (dragged_ != nullptr)
	{
		RevertDrag_();
		dragged_ = nullptr;
		dragSource_ = kNoZone_;
		dragOrigin_ = {};
	}
	hover_ = nullptr;
	hoverSource_ = kNoZone_;
	infoPanel_.Hide();
}

bool ZoneLayoutEditor::TryDeferTradeSell_(IModuleNode& node, IModuleZone* target, DirectX::XMFLOAT2 world)
{
	auto* shop = dynamic_cast<ModuleShop*>(target);
	if (shop == nullptr || shop->HitUltraArea(world) || shop->HitRefineParkSlot(world) < 3u)
	{
		return false;
	}
	if (!shop->SellsOnTradeDrop(node))
	{
		return false;
	}
	return RequestSellConfirm_(node);
}

void ZoneLayoutEditor::ResolveRelease_()
{
	if (dragged_ == nullptr)
	{
		return;
	}

	const DropEval_ eval = EvalDrop_(*dragged_);
	IModuleZone* target = ZoneAt_(eval.target);
	IModuleZone* owner = OwnerZoneOf_(dragged_);
	if (owner == nullptr)
	{
		owner = ZoneAt_(dragSource_);
	}
	if (!eval.placeable || target == nullptr || owner == nullptr)
	{
		RevertDrag_();
		return;
	}

	IModuleNode* node = dragged_;
	const DirectX::XMFLOAT2 world = WorldPosOf_(*node, dragOrigin_);
	const DropResult drop = target->EvalDrop(*node, world, dragSource_);
	if (!IsDropAccepted(drop.verdict))
	{
		RevertDrag_();
		return;
	}

	if (auto* shop = dynamic_cast<ModuleShop*>(target))
	{
		const std::size_t refineSlot = shop->HitRefineParkSlot(world);
		if (refineSlot < 3u)
		{
			if (!shop->CanParkRefine(*node, refineSlot))
			{
				RevertDrag_();
				return;
			}
			shop->ParkRefine(refineSlot, *node, owner);
			owner->SyncAllVisuals();
			return;
		}
	}

	auto* shop = dynamic_cast<ModuleShop*>(ZoneAt_(ZoneId::Shop));
	if (shop != nullptr && shop->IsShopRefineOwned(node))
	{
		if (TryDeferTradeSell_(*node, target, world))
		{
			return;
		}
		// 店有炼成 Node：Field / 仓 / 买卖框。失败仍归店、弹回原格影子。
		const std::size_t heldSlot = shop->FindRefineParkIndex(node);
		node->EndLayoutGhost();
		std::unique_ptr<IModuleNode> taken = shop->TakeNode(node);
		if (taken == nullptr || !target->TryAcceptDrop(taken, drop.localPos))
		{
			if (taken != nullptr)
			{
				shop->RestoreRefineHeld(std::move(taken), heldSlot);
			}
			RevertDrag_();
			return;
		}
		return;
	}

	if (shop != nullptr && shop->IsUltraBody(node))
	{
		if (target == shop && !shop->HitUltraArea(world))
		{
			if (RequestSellConfirm_(*node))
			{
				return;
			}
			// 买卖框：卖掉并销毁，不进货架，也不停在商店原点。
			node->EndLayoutGhost();
			shop->SellFromUltra(node);
			return;
		}
		node->EndLayoutGhost();
		// 奥义栏拖出：不扣钱、不记售出。放不下就回到原栏。
		std::unique_ptr<IModuleNode> taken = shop->TakeNode(node);
		if (taken == nullptr || !target->TryAcceptDrop(taken, drop.localPos))
		{
			if (taken != nullptr)
			{
				shop->RestoreUltra(std::move(taken));
			}
			RevertDrag_();
			return;
		}
		return;
	}

	if (shop != nullptr && shop->IsShopUltraOwned(node) && !shop->IsUltraBody(node))
	{
		if (target == shop && !shop->HitUltraArea(world))
		{
			if (RequestSellConfirm_(*node))
			{
				return;
			}
			// 买卖框：按这颗 Node 自己的卖价销毁，奥义还在。
			node->EndLayoutGhost();
			shop->SellFromUltra(node);
			return;
		}
		// 内部栏拖出或换格：不扣钱、不记售出。放不下就回到原格。
		const std::size_t home = shop->FindUltraInnerIndex(node);
		if (home == static_cast<std::size_t>(-1))
		{
			RevertDrag_();
			return;
		}
		node->EndLayoutGhost();
		shop->UnbindRefine(node);
		std::unique_ptr<IModuleNode> taken = shop->TakeNode(node);
		bool placed = false;
		if (taken != nullptr)
		{
			if (target == shop)
			{
				placed = shop->TryAcceptUltraDrop(taken, drop.localPos);
			}
			else
			{
				placed = target->TryAcceptDrop(taken, drop.localPos);
			}
		}
		if (!placed)
		{
			if (taken != nullptr)
			{
				shop->RestoreUltraInner(std::move(taken), home);
			}
			RevertDrag_();
			return;
		}
		return;
	}

	if (owner != target && TryDeferTradeSell_(*node, target, world))
	{
		return;
	}

	if (shop != nullptr)
	{
		shop->UnbindRefine(node);
	}

	if (shop != nullptr && owner == shop && target == shop && shop->IsShelfStock(node) && shop->HitUltraArea(world))
	{
		// 货架买进内部栏：不走同区 SetLocalPos，否则图标离开货卡又进不了格子。
		node->EndLayoutGhost();
		if (!shop->TryPurchaseShelfIntoUltra(node, drop.localPos))
		{
			RevertDrag_();
		}
		return;
	}

	if (owner == target)
	{
		node->SetLocalPos(drop.localPos);
		node->SetZoneOrigin(target->GetOrigin());
		target->OnSameZoneMove(*node, drop.localPos);
		node->EndLayoutGhost();
		node->SyncVisual();
		target->SyncAllVisuals();
		return;
	}

	node->EndLayoutGhost();
	const std::size_t sourceIndex = owner->FindNodeIndex(node);
	std::unique_ptr<IModuleNode> taken = owner->TakeNode(node);
	if (!target->TryAcceptDrop(taken, drop.localPos))
	{
		if (taken != nullptr)
		{
			if (auto* shopOwner = dynamic_cast<ModuleShop*>(owner);
				shopOwner != nullptr && sourceIndex < ModuleShop::kSlotCount)
			{
				shopOwner->RestoreShelfNode(std::move(taken), sourceIndex);
			}
			else
			{
				(void)owner->TryAcceptDrop(taken, dragStartLocalPos_);
			}
		}
		RevertDrag_();
		return;
	}

	if (owner->GetZoneId() != ZoneId::Shop || target->GetZoneId() == ZoneId::Shop)
	{
		return;
	}

	const int price = node->GetBuyPrice();
	if (!GameStatsCodex::TrySpendCurrency(price))
	{
		std::unique_ptr<IModuleNode> back = target->TakeNode(node);
		if (back != nullptr)
		{
			if (auto* shopOwner = dynamic_cast<ModuleShop*>(owner))
			{
				shopOwner->RestoreShelfNode(std::move(back), sourceIndex);
			}
		}
		RevertDrag_();
		return;
	}
	if (auto* shop = dynamic_cast<ModuleShop*>(owner))
	{
		shop->MarkSold(sourceIndex);
	}
}

IModuleNode* ZoneLayoutEditor::PickHover_(DirectX::XMFLOAT2 mouseGame, ZoneId& outSource) const noexcept
{
	outSource = kNoZone_;
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (IModuleZone* zone : zones_)
	{
		if (zone == nullptr)
		{
			continue;
		}
		float distSq = 1.0e9f;
		IModuleNode* hit = zone->PickAt(mouseGame, distSq);
		if (hit == nullptr)
		{
			continue;
		}
		if (best == nullptr || distSq < bestDistSq)
		{
			best = hit;
			bestDistSq = distSq;
			outSource = zone->GetZoneId();
		}
	}
	return best;
}

void ZoneLayoutEditor::SnapCursorToNode_(IModuleNode& node, DirectX::XMFLOAT3 origin, Window& hostWindow) const noexcept
{
	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const int gameX = static_cast<int>(std::lround(origin.x + local.x));
	const int gameY = static_cast<int>(std::lround(origin.y + local.y));

	int clientX = 0;
	int clientY = 0;
	if (!hostWindow.MapGameToClient(gameX, gameY, clientX, clientY))
	{
		return;
	}

	POINT pt{ clientX, clientY };
	::ClientToScreen(hostWindow.GetHwnd(), &pt);
	::SetCursorPos(pt.x, pt.y);
}

void ZoneLayoutEditor::SyncRingTransform_(IModuleNode& node, DirectX::XMFLOAT3 origin)
{
	if (ringHover_ != nullptr)
	{
		SyncOneRingTransform_(*ringHover_, node, origin);
	}
	if (ringValid_ != nullptr)
	{
		SyncOneRingTransform_(*ringValid_, node, origin);
	}
	if (ringOverlap_ != nullptr)
	{
		SyncOneRingTransform_(*ringOverlap_, node, origin);
	}
	if (ringDenied_ != nullptr)
	{
		SyncOneRingTransform_(*ringDenied_, node, origin);
	}
}

void ZoneLayoutEditor::SyncOneRingTransform_(Canvas2D& ring, IModuleNode& node, DirectX::XMFLOAT3 origin) const
{
	const DirectX::XMFLOAT2 local = node.GetLocalPos();
	const float side = node.GetIconRadius() * 2.0f + kRingPadding_;
	ring.SetPosition(DirectX::XMFLOAT3{
		origin.x + local.x,
		origin.y + local.y,
		0.0f
	});
	ring.SetScale(DirectX::XMFLOAT3{ side, side, 1.0f });
}

Canvas2D* ZoneLayoutEditor::ActiveRing_() const noexcept
{
	switch (ringKind_)
	{
	case RingKind_::Valid:
		return ringValid_.get();
	case RingKind_::Overlap:
		return ringOverlap_.get();
	case RingKind_::Denied:
		return ringDenied_.get();
	case RingKind_::Hover:
	default:
		return ringHover_.get();
	}
}