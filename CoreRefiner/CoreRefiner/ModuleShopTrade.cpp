#include "ModuleShopTrade.h"

#include "Canvas.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Collision2D.h"
#include "GameStatsCodex.h"
#include "Graphics.h"
#include "IconAtlas.h"
#include "ModuleNodeFactory.h"
#include "ModuleShopDraw.h"
#include "RenderGraph.h"
#include "TextCodex.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

using namespace ModuleShopDraw;

namespace
{
	// ---- 可刷表 ----
	// 可刷标签表。data 存放标签，count 是有效个数。
	struct ShopOffered_
	{
		ModuleNodeLabel data[ModuleNodeLabelCount()]{};
		std::size_t count{ 0 };
	};

	// 可刷表 = Label 全集 − 进化表 − Core 表 − Fusion 表 − 奥义。
	[[nodiscard]] constexpr ShopOffered_ MakeShopOffered_() noexcept
	{
		ShopOffered_ out{};
		for (std::size_t i = 0; i < ModuleNodeLabelCount(); ++i)
		{
			const auto label = static_cast<ModuleNodeLabel>(i);
			if (LabelTableContains(kEvolveLabels, label)
				|| LabelTableContains(kCoreLabels, label)
				|| LabelTableContains(kFusionLabels, label)
				|| LabelTableContains(kUltraLabels, label))
			{
				continue;
			}
			out.data[out.count++] = label;
		}
		return out;
	}

	constexpr ShopOffered_ kShopOffered_ = MakeShopOffered_();
	static_assert(kShopOffered_.count >= ModuleShopTrade::kSlotCount);

	// 打乱前 count 个标签。刷新和初次补货都用它。
	void ShuffleLabels_(ModuleNodeLabel* labels, std::size_t count)
	{
		if (count < 2u)
		{
			return;
		}
		static std::mt19937 rng{ std::random_device{}() };
		for (std::size_t i = count; i > 1u; --i)
		{
			std::uniform_int_distribution<std::size_t> dist(0u, i - 1u);
			const std::size_t j = dist(rng);
			const ModuleNodeLabel tmp = labels[i - 1u];
			labels[i - 1u] = labels[j];
			labels[j] = tmp;
		}
	}
}


// ---- 公开 · 重载 ----

void ModuleShopTrade::Tick(float dt)
{
	if (refreshDeniedSec_ > 0.0f)
	{
		refreshDeniedSec_ -= dt;
		if (refreshDeniedSec_ < 0.0f)
		{
			refreshDeniedSec_ = 0.0f;
		}
	}
	SyncHud();
}

void ModuleShopTrade::Sync()
{
	SyncTradePanelTransform_();
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncHud();
}

void ModuleShopTrade::Submit() const
{
	if (tradePanel_ != nullptr)
	{
		tradePanel_->Submit(Chan::ui);
	}
	for (const ModuleShopPanel& card : slotPanels_)
	{
		card.Submit();
	}
}

void ModuleShopTrade::SubmitOverlay() const
{
	if (currencyIcon_ != nullptr)
	{
		currencyIcon_->Submit(Chan::ui);
	}
	if (currencyText_ != nullptr)
	{
		currencyText_->Submit(Chan::ui);
	}
	if (refreshCostText_ != nullptr)
	{
		refreshCostText_->Submit(Chan::ui);
	}
	if (refreshIcon_ != nullptr)
	{
		refreshIcon_->Submit(Chan::ui);
	}
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		if (lockButtons_[i] == nullptr)
		{
			continue;
		}
		const Slot& slot = slots_[i];
		if (slot.sold || slot.node == nullptr)
		{
			continue;
		}
		lockButtons_[i]->Submit(Chan::ui);
	}
}

void ModuleShopTrade::RefreshCopy()
{
	RefreshSlotCards_();
}

bool ModuleShopTrade::Owns(const IModuleNode* node) const noexcept
{
	return FindSlotIndex(node) < kSlotCount;
}

std::unique_ptr<IModuleNode> ModuleShopTrade::TakeNode(IModuleNode* node)
{
	const std::size_t index = FindSlotIndex(node);
	if (index >= kSlotCount)
	{
		return nullptr;
	}
	std::unique_ptr<IModuleNode> taken = std::move(slots_[index].node);
	if (taken != nullptr)
	{
		taken->ClearVisualRadiusOverride();
	}
	RefreshSlotCards_();
	return taken;
}

IModuleNode* ModuleShopTrade::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;
	for (Slot& slot : slots_)
	{
		IModuleNode* node = slot.node.get();
		if (node == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 local = node->GetLocalPos();
		const DirectX::XMFLOAT2 world{
			hostOrigin_.x + local.x,
			hostOrigin_.y + local.y
		};
		const Collider2D::CircleCollider hit{ world, node->GetVisualRadius() };
		const Collider2D::PointCollider point{ worldPos };
		if (!Collider2D::CollisionSystem::IsOverlap(hit, point))
		{
			continue;
		}
		const float dx = worldPos.x - world.x;
		const float dy = worldPos.y - world.y;
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

std::size_t ModuleShopTrade::GetZoneNodeCount() const noexcept
{
	return kSlotCount;
}

IModuleNode* ModuleShopTrade::GetZoneNode(std::size_t index) const noexcept
{
	if (index >= kSlotCount)
	{
		return nullptr;
	}
	return slots_[index].node.get();
}

void ModuleShopTrade::SubmitNodes()
{
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->SubmitVisual();
		}
	}
}

void ModuleShopTrade::SyncNodes()
{
	for (Slot& slot : slots_)
	{
		IModuleNode* node = slot.node.get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetZoneOrigin(hostOrigin_);
		node->SyncVisual();
	}
}

// ---- 公开 · 独有 · 货架 ----

void ModuleShopTrade::FillStock()
{
	ModuleNodeLabel pool[ModuleNodeLabelCount()]{};
	for (std::size_t i = 0; i < kShopOffered_.count; ++i)
	{
		pool[i] = kShopOffered_.data[i];
	}
	ShuffleLabels_(pool, kShopOffered_.count);
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		RestockSlot_(i, pool[i]);
	}
	RelayoutSlots_();
}

void ModuleShopTrade::BeginVisit()
{
	ResetVisit();
	RerollStock_();
}

void ModuleShopTrade::ResetVisit()
{
	refreshCount_ = 0;
	refreshDeniedSec_ = 0.0f;
	paintedRefreshCost_ = -1;
	if (gfx_ != nullptr)
	{
		SyncHud();
	}
}

bool ModuleShopTrade::TryRefresh()
{
	if (!HasRerollableSlot_())
	{
		DenyRefresh_();
		return false;
	}

	const int cost = GetRefreshCost();
	if (!GameStatsCodex::TrySpendCurrency(cost))
	{
		DenyRefresh_();
		return false;
	}
	++refreshCount_;
	RerollStock_();
	paintedRefreshCost_ = -1;
	SyncHud();
	return true;
}

int ModuleShopTrade::GetRefreshCost() const noexcept
{
	return kRefreshBaseCost_ + refreshCount_ * kRefreshCostStep_;
}

void ModuleShopTrade::MarkSold(std::size_t index)
{
	if (index >= kSlotCount)
	{
		return;
	}
	slots_[index].sold = true;
	slots_[index].locked = false;
	slots_[index].node.reset();
	RefreshSlotCards_();
}

void ModuleShopTrade::RestoreShelfNode(std::unique_ptr<IModuleNode> node, std::size_t index)
{
	if (node == nullptr || index >= kSlotCount)
	{
		return;
	}
	node->EndLayoutGhost();
	Slot& slot = slots_[index];
	slot.node = std::move(node);
	slot.sold = false;
	slot.price = slot.node->GetBuyPrice();
	RelayoutSlots_();
}

std::size_t ModuleShopTrade::FindSlotIndex(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return kSlotCount;
	}
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		if (slots_[i].node.get() == node)
		{
			return i;
		}
	}
	return kSlotCount;
}

// ---- 公开 · 独有 · 点击 ----

bool ModuleShopTrade::HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const DirectX::XMFLOAT2 center = RefreshButtonCenter_();
	const float half = kTradeHudIconWorld * 0.5f + kTradeHudHitPad;
	return PointInAabb_(center, DirectX::XMFLOAT2{ half, half }, worldPos);
}

bool ModuleShopTrade::HitLockButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return HitLockSlotIndex_(worldPos) < kSlotCount;
}

bool ModuleShopTrade::ToggleLockAt(DirectX::XMFLOAT2 worldPos)
{
	const std::size_t index = HitLockSlotIndex_(worldPos);
	if (index >= kSlotCount)
	{
		return false;
	}

	slots_[index].locked = !slots_[index].locked;
	slotPanels_[index].Rebuild(slots_[index].node.get(), slots_[index].price, slots_[index].sold, slots_[index].locked);
	PaintLockButton_(index);
	return true;
}

// ---- 公开 · 独有 · 位置 ----

DirectX::XMFLOAT2 ModuleShopTrade::SlotLocalPos(std::size_t index) const
{
	// 图标在货卡顶区水平居中，下面才是标题和种类。
	const float radius = SlotCardIconRadius_();
	const float cardTop = CardCenterLocalY_() - kCardHeight * 0.5f;
	return DirectX::XMFLOAT2{
		CardCenterLocalX_(index),
		cardTop + ModuleShopPanel::kIconPad + radius
	};
}

void ModuleShopTrade::Relayout()
{
	RelayoutSlots_();
}

void ModuleShopTrade::SyncHud()
{
	EnsureHudVisuals_();
	PaintHudIcons_();
	if (currencyText_ != nullptr)
	{
		PaintHudNumber_(*currencyText_, GameStatsCodex::GetCurrency(), paintedCurrency_);
	}
	if (refreshCostText_ != nullptr)
	{
		PaintHudNumber_(*refreshCostText_, GetRefreshCost(), paintedRefreshCost_);
	}
	SyncHudTransforms_();
}

// ---- 私有 · 重载 ----

void ModuleShopTrade::EnsureVisuals_()
{
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}
	EnsureTradePanelVisual_(*gfx_, *rg_);
	EnsureSlotCardVisuals_(*gfx_, *rg_);
	EnsureLockButtonVisuals_(*gfx_, *rg_);
}

// ---- 私有 · 独有 · 布局 ----


float ModuleShopTrade::SlotCardIconRadius_() const
{
	return ModuleShopPanel::IconWorldRadius();
}

float ModuleShopTrade::SlotPitchX_() const noexcept
{
	const float innerW = bounds_.half.x * 2.0f;
	return (innerW - 2.0f * kSlotMargin) / static_cast<float>(kSlotCount);
}

DirectX::XMFLOAT2 ModuleShopTrade::TradeCenterLocal_() const noexcept
{
	return DirectX::XMFLOAT2{
		bounds_.center.x - hostOrigin_.x,
		bounds_.center.y - hostOrigin_.y
	};
}

float ModuleShopTrade::CardCenterLocalX_(std::size_t index) const noexcept
{
	const float pitch = SlotPitchX_();
	const float fromCenter = (static_cast<float>(index) - (static_cast<float>(kSlotCount) - 1.0f) * 0.5f) * pitch;
	return TradeCenterLocal_().x + fromCenter;
}

float ModuleShopTrade::CardCenterLocalY_() const noexcept
{
	const DirectX::XMFLOAT2 center = TradeCenterLocal_();
	return center.y - bounds_.half.y + kHudBarHeight + kSlotMargin + kCardHeight * 0.5f;
}

// ---- 私有 · 独有 · 货架 ----

void ModuleShopTrade::RelayoutSlots_()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		IModuleNode* node = slots_[i].node.get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetLocalPos(SlotLocalPos(i));
		node->SetZoneOrigin(hostOrigin_);
		node->SetVisualRadiusOverride(SlotCardIconRadius_());
		node->SyncVisual();
	}
	RefreshSlotCards_();
}

void ModuleShopTrade::RestockSlot_(std::size_t index, ModuleNodeLabel label)
{
	if (index >= kSlotCount)
	{
		return;
	}

	Slot& slot = slots_[index];
	slot.node = ModuleNodeFactory::MakeModuleNode(label, SlotLocalPos(index));
	slot.sold = false;
	slot.locked = false;
	slot.price = (slot.node != nullptr)
		? slot.node->GetBuyPrice()
		: 0;

	if (gfx_ != nullptr && rg_ != nullptr && slot.node != nullptr)
	{
		slot.node->InitVisual(*gfx_, *rg_, hostOrigin_);
	}
}

bool ModuleShopTrade::HasRerollableSlot_() const noexcept
{
	for (const Slot& slot : slots_)
	{
		const bool keepLocked = slot.locked && !slot.sold && slot.node != nullptr;
		if (!keepLocked)
		{
			return true;
		}
	}
	return false;
}

void ModuleShopTrade::DenyRefresh_()
{
	refreshDeniedSec_ = 0.35f;
	PaintHudIcons_();
}

void ModuleShopTrade::RerollStock_()
{
	bool taken[ModuleNodeLabelCount()]{};
	for (const Slot& slot : slots_)
	{
		if (slot.locked && !slot.sold && slot.node != nullptr)
		{
			taken[ToIndex(slot.node->GetModuleNodeLabel())] = true;
		}
	}

	ModuleNodeLabel pool[ModuleNodeLabelCount()]{};
	std::size_t poolN = 0;
	for (std::size_t i = 0; i < kShopOffered_.count; ++i)
	{
		const ModuleNodeLabel label = kShopOffered_.data[i];
		if (!taken[ToIndex(label)])
		{
			pool[poolN++] = label;
		}
	}
	ShuffleLabels_(pool, poolN);

	std::size_t p = 0;
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const Slot& slot = slots_[i];
		if (slot.locked && !slot.sold && slot.node != nullptr)
		{
			continue;
		}
		if (p >= poolN)
		{
			break;
		}
		RestockSlot_(i, pool[p++]);
	}
	RelayoutSlots_();
}

// ---- 私有 · 独有 · 顶栏 ----

DirectX::XMFLOAT2 ModuleShopTrade::CurrencyIconCenter_() const noexcept
{
	const float barY = bounds_.center.y - bounds_.half.y + kHudBarHeight * 0.5f;
	return DirectX::XMFLOAT2{
		bounds_.center.x - bounds_.half.x + kSlotMargin + kTradeHudIconWorld * 0.5f,
		barY
	};
}

DirectX::XMFLOAT2 ModuleShopTrade::RefreshButtonCenter_() const noexcept
{
	const float barY = bounds_.center.y - bounds_.half.y + kHudBarHeight * 0.5f;
	return DirectX::XMFLOAT2{
		bounds_.center.x + bounds_.half.x - kSlotMargin - kTradeHudIconWorld * 0.5f,
		barY
	};
}

Color ModuleShopTrade::RefreshIconTint_() const noexcept
{
	if (refreshDeniedSec_ > 0.0f)
	{
		return kTradeRefreshDenied;
	}
	if (GameStatsCodex::GetCurrency() < GetRefreshCost())
	{
		return kTradeRefreshPoor;
	}
	return kTradeRefreshOk;
}

void ModuleShopTrade::EnsureHudVisuals_()
{
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}

	auto makeIcon = [this](std::unique_ptr<Canvas2D>& canvas)
	{
		if (canvas != nullptr)
		{
			return;
		}
		canvas = std::make_unique<Canvas2D>(*gfx_, IModuleNode::kVisualSize, IModuleNode::kVisualSize);
		canvas->Clear(Colors::None);
		canvas->LinkTechniques(*rg_);
		canvas->SetScale(DirectX::XMFLOAT3{ kTradeHudIconWorld, kTradeHudIconWorld, 1.0f });
	};
	auto makeText = [this](std::unique_ptr<Canvas2D>& canvas)
	{
		if (canvas != nullptr)
		{
			return;
		}
		canvas = std::make_unique<Canvas2D>(*gfx_, 32u, 16u);
		canvas->Clear(Colors::None);
		canvas->LinkTechniques(*rg_);
	};

	makeIcon(currencyIcon_);
	makeIcon(refreshIcon_);
	makeText(currencyText_);
	makeText(refreshCostText_);
}

void ModuleShopTrade::PaintHudIcons_()
{
	EnsureHudVisuals_();
	if (currencyIcon_ == nullptr || refreshIcon_ == nullptr)
	{
		return;
	}

	auto blit = [](Canvas2D& canvas, const IconAtlas::IconBits& bits, Color color)
	{
		canvas.Clear(Colors::None);
		IconAtlas::BlitIcon(canvas, bits, color);
		canvas.NotifyPixelsChanged();
	};

	if (!currencyIconReady_)
	{
		blit(*currencyIcon_, UiIconAtlas::Get(UiIconId::Currency), kTradeCoin);
		currencyIconReady_ = true;
	}

	const Color tint = RefreshIconTint_();
	if (paintedRefreshTint_ != tint)
	{
		blit(*refreshIcon_, UiIconAtlas::Get(UiIconId::Refresh), tint);
		paintedRefreshTint_ = tint;
	}
}

void ModuleShopTrade::PaintHudNumber_(Canvas2D& canvas, int value, int& painted)
{
	if (painted == value)
	{
		return;
	}

	PaintTradeHudNumber(canvas, value);
	painted = value;
}

void ModuleShopTrade::SyncHudTransforms_() noexcept
{
	const DirectX::XMFLOAT2 coin = CurrencyIconCenter_();
	const DirectX::XMFLOAT2 refresh = RefreshButtonCenter_();

	if (currencyIcon_ != nullptr)
	{
		currencyIcon_->SetPosition(DirectX::XMFLOAT3{ coin.x, coin.y, 0.0f });
	}
	if (refreshIcon_ != nullptr)
	{
		refreshIcon_->SetPosition(DirectX::XMFLOAT3{ refresh.x, refresh.y, 0.0f });
	}
	if (currencyText_ != nullptr)
	{
		const float halfW = static_cast<float>(currencyText_->GetCanvasWidth()) * 0.5f;
		currencyText_->SetPosition(DirectX::XMFLOAT3{
			coin.x + kTradeHudIconWorld * 0.5f + kTradeHudTextGap + halfW,
			coin.y,
			0.0f
		});
	}
	if (refreshCostText_ != nullptr)
	{
		const float halfW = static_cast<float>(refreshCostText_->GetCanvasWidth()) * 0.5f;
		refreshCostText_->SetPosition(DirectX::XMFLOAT3{
			refresh.x - kTradeHudIconWorld * 0.5f - kTradeHudTextGap - halfW,
			refresh.y,
			0.0f
		});
	}
}

// ---- 私有 · 独有 · 货卡和锁 ----

DirectX::XMFLOAT2 ModuleShopTrade::LockButtonCenter_(std::size_t index) const noexcept
{
	const float cardBottom = hostOrigin_.y + CardCenterLocalY_() + kCardHeight * 0.5f;
	return DirectX::XMFLOAT2{
		std::round(hostOrigin_.x + CardCenterLocalX_(index)),
		std::round(cardBottom + kTradeLockGap + kTradeLockWorld * 0.5f)
	};
}

std::size_t ModuleShopTrade::HitLockSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const float half = kTradeLockWorld * 0.5f + kTradeHudHitPad;
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const Slot& slot = slots_[i];
		if (slot.sold || slot.node == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 center = LockButtonCenter_(i);
		if (PointInAabb_(center, DirectX::XMFLOAT2{ half, half }, worldPos))
		{
			return i;
		}
	}
	return kSlotCount;
}

void ModuleShopTrade::RefreshSlotCards_()
{
	if (gfx_ != nullptr && rg_ != nullptr)
	{
		EnsureSlotCardVisuals_(*gfx_, *rg_);
		EnsureLockButtonVisuals_(*gfx_, *rg_);
	}
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const Slot& slot = slots_[i];
		slotPanels_[i].Rebuild(slot.node.get(), slot.price, slot.sold, slot.locked);
		PaintLockButton_(i);
	}
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncHud();
}

void ModuleShopTrade::EnsureSlotCardVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	for (ModuleShopPanel& panel : slotPanels_)
	{
		panel.Ensure(gfx, rg);
	}
}

void ModuleShopTrade::SyncSlotCardTransforms_() noexcept
{
	const float cardCenterLocalY = CardCenterLocalY_();
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		slotPanels_[i].SetWorldCenter(DirectX::XMFLOAT3{
			hostOrigin_.x + CardCenterLocalX_(i),
			hostOrigin_.y + cardCenterLocalY,
			0.0f
		});
	}
}

void ModuleShopTrade::EnsureLockButtonVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned size = kTradeLockPixels;
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		EnsureCanvasSize_(lockButtons_[i], gfx, rg, size, size);
		if (lockButtons_[i] != nullptr)
		{
			lockButtons_[i]->SetScale(DirectX::XMFLOAT3{
				kTradeLockWorld,
				kTradeLockWorld,
				1.0f
			});
		}
	}
}

void ModuleShopTrade::PaintLockButton_(std::size_t index)
{
	if (index >= kSlotCount || lockButtons_[index] == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *lockButtons_[index];
	const Slot& slot = slots_[index];
	if (slot.sold || slot.node == nullptr)
	{
		canvas.Clear(Colors::None);
		canvas.NotifyPixelsChanged();
		return;
	}

	PaintTradeLockButton(
		canvas,
		UiIconAtlas::Get(slot.locked ? UiIconId::LockClosed : UiIconId::LockOpen),
		slot.locked);
}

void ModuleShopTrade::SyncLockButtonTransforms_() noexcept
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		Canvas2D* canvas = lockButtons_[i].get();
		if (canvas == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 center = LockButtonCenter_(i);
		canvas->SetPosition(DirectX::XMFLOAT3{ center.x, center.y, 0.0f });
		canvas->SetScale(DirectX::XMFLOAT3{ kTradeLockWorld, kTradeLockWorld, 1.0f });
	}
}

// ---- 私有 · 独有 · 底板 ----

void ModuleShopTrade::EnsureTradePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned width = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.x * 2.0f)));
	const unsigned height = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.y * 2.0f)));
	if (EnsureCanvasSize_(tradePanel_, gfx, rg, width, height))
	{
		PaintTradePanel_();
	}
	SyncTradePanelTransform_();
}

void ModuleShopTrade::PaintTradePanel_()
{
	if (tradePanel_ == nullptr)
	{
		return;
	}

	PaintBandChrome_(*tradePanel_);
	PaintTradeHudBar(*tradePanel_);
	tradePanel_->NotifyPixelsChanged();
}

void ModuleShopTrade::SyncTradePanelTransform_() noexcept
{
	SyncBandTransform_(tradePanel_.get(), bounds_);
}
