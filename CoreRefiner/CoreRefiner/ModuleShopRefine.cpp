#include "ModuleShopRefine.h"

#include "Canvas.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "GameRulesCodex.h"
#include "GameStatsCodex.h"
#include "Graphics.h"
#include "IconAtlas.h"
#include "ModuleNodeFactory.h"
#include "ModuleShop.h"
#include "ModuleShopDraw.h"
#include "NodeEvolveRecipes.h"
#include "RenderGraph.h"
#include "TextCodex.h"
#include "UiCopy.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

using namespace ModuleShopDraw;

static_assert(kRefineSlotCount == static_cast<int>(ModuleShopRefine::kParkCount));
static_assert(kRefineButtonCount == static_cast<int>(ModuleShopRefine::kButtonCount));

// ---- 公开 · 前置 ----

void ModuleShopRefine::SetHostZone(IModuleZone* host) noexcept
{
	hostZone_ = host;
}

// ---- 公开 · 重载 ----

void ModuleShopRefine::Sync()
{
	if (NeedsPaint_())
	{
		Paint_();
	}
	SyncTransform_();
}

void ModuleShopRefine::Submit() const
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

void ModuleShopRefine::RefreshCopy()
{
	Paint_();
}

bool ModuleShopRefine::Owns(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return false;
	}
	return node == result_.get()
		|| node == feedOwned_[0].get()
		|| node == feedOwned_[1].get();
}

std::unique_ptr<IModuleNode> ModuleShopRefine::TakeNode(IModuleNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}

	auto takeOwned = [this](std::unique_ptr<IModuleNode>& held) -> std::unique_ptr<IModuleNode>
	{
		Unbind(held.get());
		std::unique_ptr<IModuleNode> taken = std::move(held);
		if (taken != nullptr)
		{
			taken->ClearIconRadiusOverride();
			taken->ClearVisualRadiusOverride();
		}
		Paint_();
		return taken;
	};
	if (result_.get() == node)
	{
		return takeOwned(result_);
	}
	if (feedOwned_[0].get() == node)
	{
		return takeOwned(feedOwned_[0]);
	}
	if (feedOwned_[1].get() == node)
	{
		return takeOwned(feedOwned_[1]);
	}
	return nullptr;
}

IModuleNode* ModuleShopRefine::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;
	for (std::size_t i = 0; i < kParkCount; ++i)
	{
		IModuleNode* node = parked_[i];
		if (node == nullptr || HitSlotIndex_(worldPos) != i)
		{
			continue;
		}
		const DirectX::XMFLOAT2 wc = SlotWorldCenter(i);
		const float dx = worldPos.x - wc.x;
		const float dy = worldPos.y - wc.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = node;
		}
	}
	if (best != nullptr)
	{
		outDistSq = bestDistSq;
	}
	return best;
}

std::size_t ModuleShopRefine::GetZoneNodeCount() const noexcept
{
	return kZoneNodeCount;
}

IModuleNode* ModuleShopRefine::GetZoneNode(std::size_t index) const noexcept
{
	if (index == 0u)
	{
		return result_.get();
	}
	if (index >= kZoneNodeCount)
	{
		return nullptr;
	}
	return feedOwned_[index - 1u].get();
}

void ModuleShopRefine::SubmitNodes()
{
	if (result_ != nullptr)
	{
		result_->SubmitVisual();
	}
	for (std::unique_ptr<IModuleNode>& held : feedOwned_)
	{
		if (held != nullptr)
		{
			held->SubmitVisual();
		}
	}
}

void ModuleShopRefine::SyncNodes()
{
	auto syncOwned = [this](IModuleNode* node)
	{
		if (node == nullptr)
		{
			return;
		}
		node->SetZoneOrigin(hostOrigin_);
		node->SyncVisual();
	};
	syncOwned(result_.get());
	syncOwned(feedOwned_[0].get());
	syncOwned(feedOwned_[1].get());
}

// ---- 公开 · 独有 · 停放 ----

std::size_t ModuleShopRefine::HitParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return HitSlotIndex_(worldPos);
}

DirectX::XMFLOAT2 ModuleShopRefine::SlotWorldCenter(std::size_t slot) const noexcept
{
	return SlotCenterFrom_(CurrentLayout_(), slot);
}

bool ModuleShopRefine::CanPark(const IModuleNode& node, std::size_t slot) const noexcept
{
	if (slot >= kParkCount)
	{
		return false;
	}
	// 奥义本体不能当素材或主体。内部栏的 Node 可以停进来，实体仍留在奥义栏。
	if (node.GetKind() == ModuleNodeKind::Ultra)
	{
		return false;
	}
	// 满级核心类 Node 不能进炼成。
	if (node.IsCore() && node.GetLevel() >= ModuleNodeLevel::kMax)
	{
		return false;
	}
	if (slot == 2u)
	{
		// 只有店持有的炼成 Node 能回结果格。场和仓不能进。
		return Owns(&node);
	}
	if (slot == 0u && node.IsCore())
	{
		return false;
	}
	if (slot == 1u && node.GetKind() == ModuleNodeKind::Fusion)
	{
		return false;
	}
	return true;
}

void ModuleShopRefine::Park(std::size_t slot, IModuleNode& node, IModuleZone* owner)
{
	if (slot >= kParkCount)
	{
		return;
	}
	if (slot == 2u)
	{
		(void)ReturnOwnedToResult_(node);
		return;
	}
	Unbind(&node);
	if (IModuleNode* occ = parked_[slot])
	{
		if (occ != &node)
		{
			BouncePark_(*occ);
		}
	}
	RelocateOwnedToFeed_(slot, node);
	parked_[slot] = &node;
	PlacePark_(slot, node, owner);
	Paint_();
}

void ModuleShopRefine::Unbind(IModuleNode* node) noexcept
{
	if (node == nullptr)
	{
		return;
	}
	bool changed = false;
	for (IModuleNode*& slot : parked_)
	{
		if (slot == node)
		{
			slot = nullptr;
			changed = true;
		}
	}
	if (changed)
	{
		node->ClearIconRadiusOverride();
		Paint_();
	}
}

void ModuleShopRefine::ClearParks()
{
	// 素材和主体：场、仓回残影，奥义内部 Node 结束残影，店有 Node 回结果栏。结果格留下。
	for (std::size_t i = 0; i < feedOwned_.size(); ++i)
	{
		IModuleNode* node = parked_[i];
		if (node == nullptr)
		{
			continue;
		}
		BouncePark_(*node);
	}
	Paint_();
}

void ModuleShopRefine::ClearResult()
{
	for (std::size_t i = 0; i < feedOwned_.size(); ++i)
	{
		if (parked_[i] != nullptr && Owns(parked_[i]))
		{
			parked_[i] = nullptr;
		}
		feedOwned_[i].reset();
	}
	parked_[2] = nullptr;
	result_.reset();
	Paint_();
}

void ModuleShopRefine::RestoreResult(std::unique_ptr<IModuleNode> node)
{
	RestoreHeld(std::move(node), 2u);
}

void ModuleShopRefine::RestoreHeld(std::unique_ptr<IModuleNode> node, std::size_t slot)
{
	if (node == nullptr)
	{
		return;
	}
	if (slot <= 1u)
	{
		node->SetZoneVisualScale(1.0f);
		feedOwned_[slot] = std::move(node);
		IModuleNode* parked = feedOwned_[slot].get();
		parked_[slot] = parked;
		PlacePark_(slot, *parked, hostZone_);
		PinHomeGhost(*parked);
		parked->SyncVisual();
		Paint_();
		return;
	}
	AdoptResult_(std::move(node));
}

std::size_t ModuleShopRefine::FindParkIndex(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return kParkCount;
	}
	for (std::size_t i = 0; i < kParkCount; ++i)
	{
		if (parked_[i] == node)
		{
			return i;
		}
	}
	if (node == result_.get())
	{
		return 2u;
	}
	if (node == feedOwned_[0].get())
	{
		return 0u;
	}
	if (node == feedOwned_[1].get())
	{
		return 1u;
	}
	return kParkCount;
}

bool ModuleShopRefine::IsParked(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return false;
	}
	for (const IModuleNode* slot : parked_)
	{
		if (slot == node)
		{
			return true;
		}
	}
	return false;
}

bool ModuleShopRefine::IsResultParked(const IModuleNode* node) const noexcept
{
	return node != nullptr && node == result_.get();
}

IModuleNode* ModuleShopRefine::GetParked(std::size_t slot) const noexcept
{
	if (slot >= kParkCount)
	{
		return nullptr;
	}
	return parked_[slot];
}

void ModuleShopRefine::ForgetParked(const IModuleNode* node) noexcept
{
	if (node == nullptr)
	{
		return;
	}
	for (IModuleNode*& slot : parked_)
	{
		if (slot == node)
		{
			slot = nullptr;
		}
	}
}

void ModuleShopRefine::ApplyParkIcon(IModuleNode& node) noexcept
{
	node.SetIconRadiusOverride(SlotRadius_());
}

void ModuleShopRefine::PinHomeGhost(IModuleNode& node) noexcept
{
	node.SetZoneOrigin(hostOrigin_);
	const DirectX::XMFLOAT2 wc = SlotWorldCenter(2);
	node.BeginLayoutGhost(DirectX::XMFLOAT2{ wc.x - hostOrigin_.x, wc.y - hostOrigin_.y });
	node.SetVisualRadiusOverride(SlotRadius_());
}

void ModuleShopRefine::RealignResult()
{
	PlaceResultVisual_();
}

// ---- 公开 · 独有 · 按钮 ----

bool ModuleShopRefine::HitButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return HitButtonIndex_(worldPos) < kButtonCount;
}

bool ModuleShopRefine::TryClick(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse)
{
	const std::size_t i = HitButtonIndex_(worldPos);
	if (i == 0u)
	{
		return TryUpgrade_(field, warehouse);
	}
	if (i == 1u)
	{
		return TryFuse_(field, warehouse);
	}
	if (i == 2u)
	{
		return TryEvolve_(field, warehouse);
	}
	if (i == 3u)
	{
		return TryReturn_();
	}
	return false;
}

// ---- 私有 · 重载 ----

void ModuleShopRefine::EnsureVisuals_()
{
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.x * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.y * 2.0f)));
	EnsureCanvasSize_(panel_, *gfx_, *rg_, w, h);
	Paint_();
	SyncTransform_();
}

// ---- 私有 · 独有 · 布局 ----

ModuleShopRefine::Layout ModuleShopRefine::CurrentLayout_() const noexcept
{
	Layout layout{};
	layout.bounds = bounds_;
	layout.cw = (std::max)(1, static_cast<int>(std::lround(layout.bounds.half.x * 2.0f)));
	layout.ch = (std::max)(1, static_cast<int>(std::lround(layout.bounds.half.y * 2.0f)));
	if (panel_ != nullptr)
	{
		const int pw = static_cast<int>(panel_->GetCanvasWidth());
		const int ph = static_cast<int>(panel_->GetCanvasHeight());
		if (pw > 0)
		{
			layout.cw = pw;
		}
		if (ph > 0)
		{
			layout.ch = ph;
		}
	}
	// 像素位置只看画布宽高。世界矩形仍在 layout.bounds。
	FillRefineLayout(layout.cw, layout.ch, layout);
	return layout;
}

bool ModuleShopRefine::WorldToPixel_(DirectX::XMFLOAT2 world, const Layout& layout, float& px, float& py) const noexcept
{
	const float worldW = layout.bounds.half.x * 2.0f;
	const float worldH = layout.bounds.half.y * 2.0f;
	if (worldW <= 1.0e-4f || worldH <= 1.0e-4f || layout.cw <= 0 || layout.ch <= 0)
	{
		return false;
	}
	px = (world.x - (layout.bounds.center.x - layout.bounds.half.x)) * (static_cast<float>(layout.cw) / worldW);
	py = (world.y - (layout.bounds.center.y - layout.bounds.half.y)) * (static_cast<float>(layout.ch) / worldH);
	return true;
}

DirectX::XMFLOAT2 ModuleShopRefine::SlotCenterFrom_(const Layout& layout, std::size_t slot) const noexcept
{
	if (slot >= kParkCount || layout.cw <= 0 || layout.ch <= 0)
	{
		return layout.bounds.center;
	}
	const float worldW = layout.bounds.half.x * 2.0f;
	const float worldH = layout.bounds.half.y * 2.0f;
	const float px = static_cast<float>(layout.slotXs[slot]) + static_cast<float>(layout.side) * 0.5f;
	const float py = static_cast<float>(layout.slotTop) + static_cast<float>(layout.side) * 0.5f;
	return DirectX::XMFLOAT2{
		layout.bounds.center.x - layout.bounds.half.x + px * (worldW / static_cast<float>(layout.cw)),
		layout.bounds.center.y - layout.bounds.half.y + py * (worldH / static_cast<float>(layout.ch))
	};
}

float ModuleShopRefine::SlotRadiusFrom_(const Layout& layout) const noexcept
{
	if (layout.side <= 0 || layout.ch <= 0)
	{
		return kRefineIconRadiusMin;
	}
	// worldSide 是这一格边长在世界上的长度。
	const float worldSide = static_cast<float>(layout.side) * ((layout.bounds.half.y * 2.0f) / static_cast<float>(layout.ch));
	return RefineSlotIconRadius(worldSide);
}

std::size_t ModuleShopRefine::HitSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = CurrentLayout_();
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(worldPos, layout, px, py) || layout.side <= 0)
	{
		return kParkCount;
	}
	for (std::size_t i = 0; i < kParkCount; ++i)
	{
		const float x0 = static_cast<float>(layout.slotXs[i]);
		const float y0 = static_cast<float>(layout.slotTop);
		const float x1 = x0 + static_cast<float>(layout.side);
		const float y1 = y0 + static_cast<float>(layout.side);
		if (px >= x0 && px < x1 && py >= y0 && py < y1)
		{
			return i;
		}
	}
	return kParkCount;
}

std::size_t ModuleShopRefine::HitButtonIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = CurrentLayout_();
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(worldPos, layout, px, py) || layout.btnW <= 0 || layout.btnH <= 0)
	{
		return kButtonCount;
	}
	for (std::size_t i = 0; i < kButtonCount; ++i)
	{
		const float x0 = static_cast<float>(layout.btnX);
		const float y0 = static_cast<float>(layout.btnTop + static_cast<int>(i) * (layout.btnH + layout.btnGap));
		const float x1 = x0 + static_cast<float>(layout.btnW);
		const float y1 = y0 + static_cast<float>(layout.btnH);
		if (px >= x0 && px < x1 && py >= y0 && py < y1)
		{
			return i;
		}
	}
	return kButtonCount;
}

float ModuleShopRefine::SlotRadius_() const noexcept
{
	return SlotRadiusFrom_(CurrentLayout_());
}

// ---- 私有 · 独有 · 绘制 ----

bool ModuleShopRefine::NeedsPaint_() const noexcept
{
	const int cur = GameStatsCodex::GetCurrency();
	return CanUpgrade_() != paintedUpgradeLit_
		|| CanFuse_() != paintedFuseLit_
		|| CanEvolve_() != paintedEvolveLit_
		|| CanReturn_() != paintedReturnLit_
		|| cur != paintedCurrency_;
}

void ModuleShopRefine::Paint_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *panel_;
	PaintBandChrome_(canvas);

	for (IModuleNode* node : parked_)
	{
		if (node == nullptr)
		{
			continue;
		}
		ApplyParkIcon(*node);
		if (node == result_.get())
		{
			node->SetVisualRadiusOverride(SlotRadius_());
		}
	}

	const Layout layout = CurrentLayout_();
	const int side = layout.side;
	const int slotTop = layout.slotTop;
	const int* slotXs = layout.slotXs;
	const int regionGap = (side > 0) ? (slotXs[1] - slotXs[0] - side) : 64;
	const int btnW = layout.btnW;
	const int btnH = layout.btnH;
	const int btnX = layout.btnX;
	const int btnTop = layout.btnTop;
	const int btnGap = layout.btnGap;

	constexpr Color kSlotText{ 220u, 228u, 236u, 255u };
	constexpr Color kArrow{ 180u, 195u, 215u, 220u };
	const std::string slotLabels[kParkCount] = {
		GetUiCopy("shop.refine.material"),
		GetUiCopy("shop.refine.subject"),
		GetUiCopy("shop.refine.result")
	};
	const UiIconId arrowIds[2] = { UiIconId::RefineFeed, UiIconId::RefineYield };
	// 素材、主体、结果是直角正方形。右侧四键才圆角。
	for (std::size_t i = 0; i < kParkCount; ++i)
	{
		const int x0 = slotXs[i];
		const int y0 = slotTop;
		DrawSlotSquare_(canvas, x0, y0, side);
		if (parked_[i] != nullptr)
		{
			// 格内有 Node 时不画素材、主体、结果标题，避免压在图标上。
			continue;
		}
		DrawShopLabelInBox_(
			canvas,
			slotLabels[i],
			kSlotLabelFont_,
			x0,
			y0,
			side,
			side,
			kSlotText);
	}

	for (int a = 0; a < 2; ++a)
	{
		const int gx = slotXs[a] + side;
		BlitIconInBox_(
			canvas,
			UiIconAtlas::Get(arrowIds[a]),
			kArrow,
			gx,
			slotTop,
			regionGap,
			side);
	}

	const std::string btnLabels[kButtonCount] = {
		GetUiCopy("shop.refine.upgrade"),
		GetUiCopy("shop.refine.fuse"),
		GetUiCopy("shop.refine.evolve"),
		GetUiCopy("shop.refine.return")
	};
	const bool btnLit[kButtonCount] = {
		CanUpgrade_(),
		CanFuse_(),
		CanEvolve_(),
		CanReturn_()
	};
	for (std::size_t i = 0; i < kButtonCount; ++i)
	{
		const int x0 = btnX;
		const int y0 = btnTop + static_cast<int>(i) * (btnH + btnGap);
		// 升级、合成、进化常驻费用。退回不标价。
		const int cost = (i < 3u) ? GameRulesCodex::RefineOpCost : -1;
		DrawActionButton_(canvas, x0, y0, btnW, btnH, btnLabels[i], cost, btnLit[i]);
	}

	paintedUpgradeLit_ = btnLit[0];
	paintedFuseLit_ = btnLit[1];
	paintedEvolveLit_ = btnLit[2];
	paintedReturnLit_ = btnLit[3];
	paintedCurrency_ = GameStatsCodex::GetCurrency();
	canvas.NotifyPixelsChanged();
}

void ModuleShopRefine::SyncTransform_() noexcept
{
	SyncBandTransform_(panel_.get(), bounds_);
}

// ---- 私有 · 独有 · 停放 ----

void ModuleShopRefine::PlacePark_(std::size_t slot, IModuleNode& node, IModuleZone* owner)
{
	if (slot >= kParkCount)
	{
		return;
	}
	const Layout layout = CurrentLayout_();
	const DirectX::XMFLOAT3 origin = (owner != nullptr) ? owner->GetOrigin() : hostOrigin_;
	const DirectX::XMFLOAT2 wc = SlotCenterFrom_(layout, slot);
	const float r = SlotRadiusFrom_(layout);
	node.SetLocalPos(DirectX::XMFLOAT2{ wc.x - origin.x, wc.y - origin.y });
	node.SetZoneOrigin(origin);
	node.SetIconRadiusOverride(r);
	if (slot == 2u)
	{
		node.EndLayoutGhost();
		node.SetVisualRadiusOverride(r);
	}
	node.SyncVisual();
}

void ModuleShopRefine::BouncePark_(IModuleNode& node)
{
	if (ReturnOwnedToResult_(node))
	{
		return;
	}
	Unbind(&node);
	if (node.IsLayoutGhostActive())
	{
		node.SetLocalPos(node.GetCollisionLocalPos());
	}
	node.EndLayoutGhost();
	node.SyncVisual();
}

void ModuleShopRefine::RelocateOwnedToFeed_(std::size_t slot, IModuleNode& node)
{
	if (slot > 1u)
	{
		return;
	}

	std::unique_ptr<IModuleNode> held;
	if (result_.get() == &node)
	{
		parked_[2] = nullptr;
		held = std::move(result_);
	}
	else if (feedOwned_[0].get() == &node && slot != 0u)
	{
		held = std::move(feedOwned_[0]);
	}
	else if (feedOwned_[1].get() == &node && slot != 1u)
	{
		held = std::move(feedOwned_[1]);
	}
	if (held != nullptr)
	{
		feedOwned_[slot] = std::move(held);
	}

	IModuleNode* parked = feedOwned_[slot].get();
	if (parked != &node)
	{
		return;
	}
	PinHomeGhost(*parked);
}

bool ModuleShopRefine::ReturnOwnedToResult_(IModuleNode& node)
{
	std::unique_ptr<IModuleNode> held;
	if (feedOwned_[0].get() == &node)
	{
		held = std::move(feedOwned_[0]);
	}
	else if (feedOwned_[1].get() == &node)
	{
		held = std::move(feedOwned_[1]);
	}
	else if (result_.get() == &node)
	{
		PlaceResultVisual_();
		Paint_();
		return true;
	}
	else
	{
		return false;
	}

	Unbind(&node);
	AdoptResult_(std::move(held));
	return true;
}

void ModuleShopRefine::AdoptResult_(std::unique_ptr<IModuleNode> node)
{
	if (node == nullptr)
	{
		return;
	}
	node->EndLayoutGhost();
	node->SetZoneVisualScale(1.0f);
	result_ = std::move(node);
	parked_[2] = result_.get();
	PlaceResultVisual_();
	Paint_();
}

void ModuleShopRefine::PlaceResultVisual_()
{
	IModuleNode* node = result_.get();
	if (node == nullptr)
	{
		return;
	}
	PlacePark_(2u, *node, hostZone_);
}

// ---- 私有 · 独有 · 操作 ----

bool ModuleShopRefine::HasPairReady_() const noexcept
{
	if (parked_[0] == nullptr || parked_[1] == nullptr)
	{
		return false;
	}
	if (parked_[2] != nullptr || result_ != nullptr)
	{
		return false;
	}
	return GameStatsCodex::GetCurrency() >= GameRulesCodex::RefineOpCost;
}

bool ModuleShopRefine::CanUpgrade_() const noexcept
{
	// 素材等级不低于主体，主体未满级。不看 Label。
	if (!HasPairReady_())
	{
		return false;
	}
	IModuleNode* material = parked_[0];
	IModuleNode* subject = parked_[1];
	if (subject->GetLevel() >= ModuleNodeLevel::kMax)
	{
		return false;
	}
	return material->GetLevel() >= subject->GetLevel();
}

bool ModuleShopRefine::CanFuse_() const noexcept
{
	// 两边都是满级、Label 不同、都不是融合。核心不能合成。
	if (!HasPairReady_())
	{
		return false;
	}
	IModuleNode* material = parked_[0];
	IModuleNode* subject = parked_[1];
	if (material->IsCore() || subject->IsCore())
	{
		return false;
	}
	if (material->GetKind() == ModuleNodeKind::Fusion || subject->GetKind() == ModuleNodeKind::Fusion)
	{
		return false;
	}
	if (material->GetLevel() < ModuleNodeLevel::kMax || subject->GetLevel() < ModuleNodeLevel::kMax)
	{
		return false;
	}
	return material->GetModuleNodeLabel() != subject->GetModuleNodeLabel();
}

bool ModuleShopRefine::CanEvolve_() const noexcept
{
	// 两边都是满级、对上进化表。和合成分开判断。
	if (!HasPairReady_())
	{
		return false;
	}
	IModuleNode* material = parked_[0];
	IModuleNode* subject = parked_[1];
	if (material->GetLevel() < ModuleNodeLevel::kMax || subject->GetLevel() < ModuleNodeLevel::kMax)
	{
		return false;
	}
	ModuleNodeLabel product = ModuleNodeLabel::Count;
	return NodeEvolveRecipes::TryFind(
		material->GetModuleNodeLabel(),
		subject->GetModuleNodeLabel(),
		product);
}

bool ModuleShopRefine::CanReturn_() const noexcept
{
	return parked_[0] != nullptr || parked_[1] != nullptr;
}

IModuleZone* ModuleShopRefine::FindOwner_(IModuleNode* node, IModuleZone* field, IModuleZone* warehouse) const noexcept
{
	// 宿主放在场、仓前面。奥义和店有炼成由 FindNodeOwnerZone 先认给商店。
	IModuleZone* zones[3]{
		hostZone_,
		field,
		warehouse
	};
	return FindNodeOwnerZone(node, zones, 3);
}

bool ModuleShopRefine::TryTakePair_(IModuleZone* field, IModuleZone* warehouse, TakenPair_& out)
{
	IModuleNode* material = parked_[0];
	IModuleNode* subject = parked_[1];
	if (material == nullptr || subject == nullptr)
	{
		return false;
	}

	out.materialOwner = FindOwner_(material, field, warehouse);
	out.subjectOwner = FindOwner_(subject, field, warehouse);
	if (out.materialOwner == nullptr || out.subjectOwner == nullptr)
	{
		return false;
	}
	if (!GameStatsCodex::TrySpendCurrency(GameRulesCodex::RefineOpCost))
	{
		return false;
	}

	Unbind(material);
	Unbind(subject);
	out.material = out.materialOwner->TakeNode(material);
	out.subject = out.subjectOwner->TakeNode(subject);
	if (out.material == nullptr || out.subject == nullptr)
	{
		Paint_();
		return false;
	}

	out.material->EndLayoutGhost();
	out.subject->EndLayoutGhost();
	out.material->ClearIconRadiusOverride();
	out.subject->ClearIconRadiusOverride();
	out.material->ClearVisualRadiusOverride();
	out.subject->ClearVisualRadiusOverride();
	return true;
}

bool ModuleShopRefine::TryUpgrade_(IModuleZone* field, IModuleZone* warehouse)
{
	if (!CanUpgrade_())
	{
		return false;
	}

	TakenPair_ taken;
	if (!TryTakePair_(field, warehouse, taken))
	{
		return false;
	}

	const int newLevel = (std::max)(taken.subject->GetLevel() + 1, taken.material->GetLevel());
	const int newPrice = taken.subject->GetBuyPrice() + taken.material->GetBuyPrice();
	taken.material.reset();
	taken.subject->SetLevel(newLevel);
	taken.subject->SetBuyPrice(newPrice);
	AdoptResult_(std::move(taken.subject));
	taken.materialOwner->SyncAllVisuals();
	if (taken.subjectOwner != taken.materialOwner)
	{
		taken.subjectOwner->SyncAllVisuals();
	}
	return true;
}

bool ModuleShopRefine::TryFuse_(IModuleZone* field, IModuleZone* warehouse)
{
	if (!CanFuse_())
	{
		return false;
	}
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return false;
	}

	TakenPair_ taken;
	if (!TryTakePair_(field, warehouse, taken))
	{
		return false;
	}

	const DirectX::XMFLOAT2 wc = SlotWorldCenter(2);
	const DirectX::XMFLOAT2 local{ wc.x - hostOrigin_.x, wc.y - hostOrigin_.y };
	std::unique_ptr<IModuleNode> fusion = ModuleNodeFactory::MakeFusion(
		std::move(taken.subject),
		std::move(taken.material),
		local);
	if (fusion == nullptr)
	{
		Paint_();
		return false;
	}

	fusion->InitVisual(*gfx_, *rg_, hostOrigin_);
	AdoptResult_(std::move(fusion));
	taken.materialOwner->SyncAllVisuals();
	if (taken.subjectOwner != taken.materialOwner)
	{
		taken.subjectOwner->SyncAllVisuals();
	}
	return true;
}

bool ModuleShopRefine::TryEvolve_(IModuleZone* field, IModuleZone* warehouse)
{
	if (!CanEvolve_())
	{
		return false;
	}
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return false;
	}

	IModuleNode* material = parked_[0];
	IModuleNode* subject = parked_[1];
	ModuleNodeLabel productLabel = ModuleNodeLabel::Count;
	if (!NodeEvolveRecipes::TryFind(
		material->GetModuleNodeLabel(),
		subject->GetModuleNodeLabel(),
		productLabel))
	{
		return false;
	}

	TakenPair_ taken;
	if (!TryTakePair_(field, warehouse, taken))
	{
		return false;
	}

	const DirectX::XMFLOAT2 wc = SlotWorldCenter(2);
	const DirectX::XMFLOAT2 local{ wc.x - hostOrigin_.x, wc.y - hostOrigin_.y };
	std::unique_ptr<IModuleNode> product = ModuleNodeFactory::MakeModuleNode(productLabel, local);
	if (product == nullptr)
	{
		Paint_();
		return false;
	}

	// 1 级新实例。造价与升级、合成相同，为两颗买入价之和。
	product->SetBuyPrice(taken.material->GetBuyPrice() + taken.subject->GetBuyPrice());
	product->InitVisual(*gfx_, *rg_, hostOrigin_);
	AdoptResult_(std::move(product));
	taken.materialOwner->SyncAllVisuals();
	if (taken.subjectOwner != taken.materialOwner)
	{
		taken.subjectOwner->SyncAllVisuals();
	}
	return true;
}

bool ModuleShopRefine::TryReturn_()
{
	if (!CanReturn_())
	{
		return false;
	}
	ClearParks();
	return true;
}
