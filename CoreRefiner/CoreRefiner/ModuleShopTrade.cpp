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

	// 把锁图标按整数倍率画在画布正中。
	void BlitLockIconCentered_(Canvas& canvas, const IconAtlas::IconBits& bits, Color color, unsigned scale)
	{
		if (scale == 0u)
		{
			return;
		}

		const int glyph = static_cast<int>(16u * scale);
		const int ox = (static_cast<int>(canvas.GetCanvasWidth()) - glyph) / 2;
		const int oy = (static_cast<int>(canvas.GetCanvasHeight()) - glyph) / 2;
		for (unsigned y = 0u; y < 16u; ++y)
		{
			const std::uint16_t row = bits[y];
			for (unsigned x = 0u; x < 16u; ++x)
			{
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) == 0)
				{
					continue;
				}
				for (unsigned sy = 0u; sy < scale; ++sy)
				{
					for (unsigned sx = 0u; sx < scale; ++sx)
					{
						CanvasPixelDraw::PutPixelClamped(
							canvas,
							ox + static_cast<int>(x * scale + sx),
							oy + static_cast<int>(y * scale + sy),
							color);
					}
				}
			}
		}
	}
}

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

int ModuleShopTrade::GetRefreshCost() const noexcept
{
	return kRefreshBaseCost_ + refreshCount_ * kRefreshCostStep_;
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

Color ModuleShopTrade::RefreshIconTint_() const noexcept
{
	if (refreshDeniedSec_ > 0.0f)
	{
		return Color{ 230u, 80u, 80u, 255u };
	}
	if (GameStatsCodex::GetCurrency() < GetRefreshCost())
	{
		return Color{ 160u, 160u, 160u, 255u };
	}
	return Color{ 120u, 220u, 180u, 255u };
}

DirectX::XMFLOAT2 ModuleShopTrade::CurrencyIconCenter_() const noexcept
{
	const float barY = bounds_.center.y - bounds_.half.y + kHudBarHeight * 0.5f;
	return DirectX::XMFLOAT2{
		bounds_.center.x - bounds_.half.x + kSlotMargin + kHudIconWorld_ * 0.5f,
		barY
	};
}

DirectX::XMFLOAT2 ModuleShopTrade::RefreshButtonCenter_() const noexcept
{
	const float barY = bounds_.center.y - bounds_.half.y + kHudBarHeight * 0.5f;
	return DirectX::XMFLOAT2{
		bounds_.center.x + bounds_.half.x - kSlotMargin - kHudIconWorld_ * 0.5f,
		barY
	};
}

bool ModuleShopTrade::HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const DirectX::XMFLOAT2 center = RefreshButtonCenter_();
	const float half = kHudIconWorld_ * 0.5f + kHudHitPad_;
	return PointInAabb_(center, DirectX::XMFLOAT2{ half, half }, worldPos);
}

std::size_t ModuleShopTrade::HitLockSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const float half = kLockButtonWorld_ * 0.5f + kHudHitPad_;
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

void ModuleShopTrade::Relayout()
{
	RelayoutSlots_();
}

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
		canvas->SetScale(DirectX::XMFLOAT3{ kHudIconWorld_, kHudIconWorld_, 1.0f });
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
		blit(*currencyIcon_, UiIconAtlas::Get(UiIconId::Currency), Color{ 255u, 210u, 80u, 255u });
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

	auto ctx = TextCodex::Get().BeginDraw();
	Text::RenderRequest& rq = ctx.Request();
	FillUiLabelRequest_(rq, std::to_string(value), kPriceFontSize, kMoneyYellow);
	rq.canvasMode = Text::CanvasMode::Auto;
	rq.clearMode = Text::ClearMode::Clear;
	rq.style.wordWrapEnabled = false;
	rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	rq.maxWidthPx = 80.0f;
	rq.paddingPx = 2;
	rq.backgroundColor = Colors::None;
	ctx.Render(canvas);

	const unsigned width = (std::max)(1u, canvas.GetCanvasWidth());
	const unsigned height = (std::max)(1u, canvas.GetCanvasHeight());
	canvas.SetScale(DirectX::XMFLOAT3{
		static_cast<float>(width),
		static_cast<float>(height),
		1.0f
	});
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
			coin.x + kHudIconWorld_ * 0.5f + 4.0f + halfW,
			coin.y,
			0.0f
		});
	}
	if (refreshCostText_ != nullptr)
	{
		const float halfW = static_cast<float>(refreshCostText_->GetCanvasWidth()) * 0.5f;
		refreshCostText_->SetPosition(DirectX::XMFLOAT3{
			refresh.x - kHudIconWorld_ * 0.5f - 4.0f - halfW,
			refresh.y,
			0.0f
		});
	}
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

DirectX::XMFLOAT2 ModuleShopTrade::LockButtonCenter_(std::size_t index) const noexcept
{
	const float cardBottom = hostOrigin_.y + CardCenterLocalY_() + kCardHeight * 0.5f;
	return DirectX::XMFLOAT2{
		std::round(hostOrigin_.x + CardCenterLocalX_(index)),
		std::round(cardBottom + kLockButtonGap_ + kLockButtonWorld_ * 0.5f)
	};
}

void ModuleShopTrade::EnsureLockButtonVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned size = kLockButtonPixels_;
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		EnsureCanvasSize_(lockButtons_[i], gfx, rg, size, size);
		if (lockButtons_[i] != nullptr)
		{
			lockButtons_[i]->SetScale(DirectX::XMFLOAT3{
				kLockButtonWorld_,
				kLockButtonWorld_,
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

	constexpr Color kBtnBg{ 42u, 56u, 78u, 255u };
	constexpr Color kBtnBgLocked{ 22u, 30u, 42u, 255u };
	constexpr Color kBtnFrame{ 170u, 190u, 210u, 255u };
	constexpr Color kBtnFrameLocked{ 88u, 104u, 124u, 255u };
	constexpr Color kIcon{ 200u, 214u, 230u, 255u };

	canvas.Clear(slot.locked ? kBtnBgLocked : kBtnBg);
	const int canvasW = static_cast<int>(canvas.GetCanvasWidth());
	const int canvasH = static_cast<int>(canvas.GetCanvasHeight());
	BlitLockIconCentered_(
		canvas,
		UiIconAtlas::Get(slot.locked ? UiIconId::LockClosed : UiIconId::LockOpen),
		kIcon,
		kLockIconScale_);
	const Color frame = slot.locked ? kBtnFrameLocked : kBtnFrame;
	CanvasPixelDraw::DrawRectBorder(canvas, 0u, 0u, static_cast<unsigned>(canvasW) - 1u, static_cast<unsigned>(canvasH) - 1u, kLockBorderTexels_, frame);
	canvas.NotifyPixelsChanged();
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
		canvas->SetScale(DirectX::XMFLOAT3{ kLockButtonWorld_, kLockButtonWorld_, 1.0f });
	}
}

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
	const int width = static_cast<int>(tradePanel_->GetCanvasWidth());
	constexpr Color kHudBar{ 28u, 38u, 56u, 200u };
	constexpr Color kFrame{ 170u, 190u, 210u, 90u };
	const int barBottom = static_cast<int>(std::lround(kHudBarHeight)) - 1;
	for (int y = 1; y < barBottom; ++y)
	{
		CanvasPixelDraw::DrawHLine(*tradePanel_, 1, width - 2, y, kHudBar);
	}
	CanvasPixelDraw::DrawHLine(*tradePanel_, 1, width - 2, barBottom, kFrame);
	tradePanel_->NotifyPixelsChanged();
}

void ModuleShopTrade::SyncTradePanelTransform_() noexcept
{
	SyncBandTransform_(tradePanel_.get(), bounds_);
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