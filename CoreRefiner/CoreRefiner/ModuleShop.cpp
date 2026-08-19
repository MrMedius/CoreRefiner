#include "ModuleShop.h"
#include "Channels.h"
#include "Collision2D.h"
#include "Colors.h"
#include "GameStatsCodex.h"
#include "Graphics.h"
#include "IconAtlas.h"
#include "ModuleNodeFactory.h"
#include "ModuleNodeInfoCopy.h"
#include "ModuleNodeLabel.h"
#include "ModuleNodePrice.h"
#include "RenderGraph.h"
#include "TextCodex.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <utility>

[[nodiscard]] static ModuleNodeLabel PickRandomShopLabel_()
{
	static std::mt19937 rng{ std::random_device{}() };
	static std::uniform_int_distribution<int> dist(
		0, static_cast<int>(ModuleNodeLabelCount()) - 1);
	return static_cast<ModuleNodeLabel>(dist(rng));
}

static_assert(ModuleShop::kSlotCount <= ModuleNodeLabelCount());

namespace
{
	void EnsureModuleNodeInfoCopyLoaded_()
	{
		if (IsModuleNodeInfoCopyLoaded())
		{
			return;
		}
		if (LoadModuleNodeInfoCopy("ModuleNodeInfoCopy.json"))
		{
			return;
		}
		if (LoadModuleNodeInfoCopy("CoreRefiner/ModuleNodeInfoCopy.json"))
		{
			return;
		}
		(void)LoadModuleNodeInfoCopy("CoreRefiner/CoreRefiner/ModuleNodeInfoCopy.json");
	}
}

float ModuleShop::HalfSpanX_() noexcept
{
	return (static_cast<float>(kColumns - 1) * 0.5f) * kSlotPitch;
}

float ModuleShop::HalfSpanY_() noexcept
{
	return (static_cast<float>(kRows - 1) * 0.5f) * kSlotPitch;
}

DirectX::XMFLOAT2 ModuleShop::SlotLocalPos_(std::size_t index) noexcept
{
	const int col = static_cast<int>(index % static_cast<std::size_t>(kColumns));
	const int row = static_cast<int>(index / static_cast<std::size_t>(kColumns));
	const float x = (static_cast<float>(col) - (static_cast<float>(kColumns - 1) * 0.5f)) * kSlotPitch;
	const float y = static_cast<float>(row) * kSlotPitch;
	return DirectX::XMFLOAT2{ x, y };
}

std::size_t ModuleShop::FindSlotIndex_(const IModuleNode* node) const noexcept
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

ModuleShop::BoundsWorld ModuleShop::GetTradeBoundsWorld() const noexcept
{
	const float halfSpanX = HalfSpanX_();
	const float halfSpanY = HalfSpanY_();
	const float localCenterY = halfSpanY;

	BoundsWorld b{};
	b.center = DirectX::XMFLOAT2{
		origin_.x,
		origin_.y + localCenterY
	};
	b.half = DirectX::XMFLOAT2{
		halfSpanX + kBoundsPad,
		halfSpanY + kBoundsPad
	};
	return b;
}

ModuleShop::BoundsWorld ModuleShop::GetShellBoundsWorld() const noexcept
{
	const BoundsWorld trade = GetTradeBoundsWorld();
	const float tradeH = trade.half.y * 2.0f;
	const float below =
		kReserveGap + kReservePanelHeight
		+ kReserveGap + kReservePanelHeight;
	const float shellH = kShellPadTop + tradeH + below;

	BoundsWorld b{};
	b.half = DirectX::XMFLOAT2{
		trade.half.x + kShellPadX,
		shellH * 0.5f
	};
	const float tradeTop = trade.center.y - trade.half.y;
	const float shellTop = tradeTop - kShellPadTop;
	b.center = DirectX::XMFLOAT2{
		trade.center.x,
		shellTop + b.half.y
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
	const BoundsWorld b = GetTradeBoundsWorld();
	const float minX = b.center.x - b.half.x;
	const float maxX = b.center.x + b.half.x;
	const float minY = b.center.y - b.half.y;
	const float maxY = b.center.y + b.half.y;
	return worldCenter.x >= minX && worldCenter.x <= maxX
		&& worldCenter.y >= minY && worldCenter.y <= maxY;
}

void ModuleShop::SetOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	origin_ = origin;
	RelayoutSlots_();
	SyncPanelTransform_();
}

void ModuleShop::FillStock()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const ModuleNodeLabel label = static_cast<ModuleNodeLabel>(i);
		slots_[i].node = ModuleNodeFactory::MakeModuleNode(label, SlotLocalPos_(i));
		slots_[i].sold = false;
		slots_[i].price = (slots_[i].node != nullptr)
			? ModuleNodePrice::GetBuyPrice(label)
			: 0;
	}
	RelayoutSlots_();
}

int ModuleShop::GetRefreshCost() const noexcept
{
	return kRefreshBaseCost_ + refreshCount_ * kRefreshCostStep_;
}

void ModuleShop::ResetVisit()
{
	refreshCount_ = 0;
	refreshDeniedSec_ = 0.0f;
	paintedRefreshCost_ = -1;
	if (gfx_ != nullptr)
	{
		SyncHud();
	}
}

void ModuleShop::TickHud(float dt)
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

bool ModuleShop::TryRefresh()
{
	const int cost = GetRefreshCost();
	if (!GameStatsCodex::TrySpendCurrency(cost))
	{
		refreshDeniedSec_ = 0.35f;
		PaintHudIcons_();
		return false;
	}
	++refreshCount_;
	RerollStock_();
	paintedRefreshCost_ = -1;
	SyncHud();
	return true;
}

void ModuleShop::RerollStock_()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const ModuleNodeLabel label = PickRandomShopLabel_();
		slots_[i].node = ModuleNodeFactory::MakeModuleNode(label, SlotLocalPos_(i));
		slots_[i].sold = false;
		slots_[i].price = (slots_[i].node != nullptr)
			? ModuleNodePrice::GetBuyPrice(label)
			: 0;
	}
	if (gfx_ != nullptr && rg_ != nullptr)
	{
		for (Slot& slot : slots_)
		{
			if (slot.node != nullptr)
			{
				slot.node->InitVisual(*gfx_, *rg_, origin_);
			}
		}
	}
	for (int& painted : paintedPrice_)
	{
		painted = -1;
	}
	RelayoutSlots_();
}

Color ModuleShop::RefreshIconTint_() const noexcept
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

DirectX::XMFLOAT2 ModuleShop::CurrencyIconCenter_() const noexcept
{
	const BoundsWorld b = GetBoundsWorld();
	return DirectX::XMFLOAT2{
		b.center.x - b.half.x + kHudIconWorld_ * 0.5f,
		b.center.y - b.half.y - kHudIconWorld_ * 0.5f - 8.0f
	};
}

DirectX::XMFLOAT2 ModuleShop::RefreshButtonCenter_() const noexcept
{
	const BoundsWorld b = GetBoundsWorld();
	return DirectX::XMFLOAT2{
		b.center.x + b.half.x - kHudIconWorld_ * 0.5f,
		b.center.y - b.half.y - kHudIconWorld_ * 0.5f - 8.0f
	};
}

bool ModuleShop::HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const DirectX::XMFLOAT2 c = RefreshButtonCenter_();
	const float half = kHudIconWorld_ * 0.5f + kHudHitPad_;
	return std::fabs(worldPos.x - c.x) <= half && std::fabs(worldPos.y - c.y) <= half;
}

void ModuleShop::EnsureHudVisuals_()
{
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}

	auto makeIcon = [&](std::unique_ptr<Canvas2D>& canvas)
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
	auto makeText = [&](std::unique_ptr<Canvas2D>& canvas)
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

void ModuleShop::PaintHudIcons_()
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

void ModuleShop::PaintHudNumber_(Canvas2D& canvas, int value, int& painted)
{
	if (painted == value)
	{
		return;
	}

	auto ctx = TextCodex::Get().BeginDraw();
	Text::RenderRequest& rq = ctx.Request();
	rq.text = std::to_string(value);
	rq.canvasMode = Text::CanvasMode::Auto;
	rq.clearMode = Text::ClearMode::Clear;
	rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	rq.fallbackFonts.clear();
	rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
	rq.style.fontSize = kPriceFontSize;
	rq.style.wordWrapEnabled = false;
	rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	rq.maxWidthPx = 80.0f;
	rq.paddingPx = 2;
	rq.defaultColor = Colors::White;
	rq.backgroundColor = Color{ 24u, 26u, 32u, 220u };
	ctx.Render(canvas);

	const unsigned w = (std::max)(1u, canvas.GetCanvasWidth());
	const unsigned h = (std::max)(1u, canvas.GetCanvasHeight());
	canvas.SetScale(DirectX::XMFLOAT3{
		static_cast<float>(w),
		static_cast<float>(h),
		1.0f
	});
	painted = value;
}

void ModuleShop::SyncHudTransforms_() noexcept
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

void ModuleShop::SyncHud()
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

void ModuleShop::SubmitHud()
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
}

void ModuleShop::MarkSold(std::size_t index)
{
	if (index >= kSlotCount)
	{
		return;
	}
	slots_[index].sold = true;
	slots_[index].node.reset();
	SyncInfoPanels_();
}

void ModuleShop::RelayoutSlots_()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		IModuleNode* node = slots_[i].node.get();
		if (node == nullptr)
		{
			continue;
		}
		node->SetLocalPos(SlotLocalPos_(i));
		node->SetZoneOrigin(origin_);
		node->SetVisualRadiusOverride(kStoredVisualRadius);
		node->SyncVisual();
	}
	SyncInfoPanels_();
}

bool ModuleShop::TryAcceptDrop(std::unique_ptr<IModuleNode>& node, DirectX::XMFLOAT2 localPos)
{
	(void)localPos;
	if (node == nullptr)
	{
		return false;
	}

	for (Slot& slot : slots_)
	{
		if (!slot.sold && slot.node == nullptr)
		{
			slot.node = std::move(node);
			RelayoutSlots_();
			return true;
		}
	}

	if (node->IsCore())
	{
		return false;
	}

	GameStatsCodex::AddCurrency(ModuleNodePrice::GetSellPrice(node->GetModuleNodeLabel()));
	node.reset();
	return true;
}

DropResult ModuleShop::EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, ZoneId from) const noexcept
{
	DropResult result{};
	if (!ContainsCircle(worldPos, node.GetHitRadius()))
	{
		result.verdict = DropVerdict::OutOfBounds;
		return result;
	}

	if (from == ZoneId::Shop)
	{
		result.verdict = DropVerdict::Accept;
		const std::size_t i = FindSlotIndex_(&node);
		if (i < kSlotCount)
		{
			result.localPos = SlotLocalPos_(i);
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

std::unique_ptr<IModuleNode> ModuleShop::TakeNode(IModuleNode* node)
{
	const std::size_t i = FindSlotIndex_(node);
	if (i >= kSlotCount)
	{
		return nullptr;
	}
	std::unique_ptr<IModuleNode> taken = std::move(slots_[i].node);
	if (taken != nullptr)
	{
		taken->ClearVisualRadiusOverride();
	}
	SyncInfoPanels_();
	return taken;
}

void ModuleShop::SyncInfoPanels_()
{
	SyncPriceLabels_();

	if (gfx_ != nullptr && rg_ != nullptr)
	{
		for (NodeInfoPanel& panel : infoPanels_)
		{
			panel.Ensure(*gfx_, *rg_);
		}
	}

	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		NodeInfoPanel& panel = infoPanels_[i];
		const Slot& slot = slots_[i];
		if (slot.sold || slot.node == nullptr)
		{
			panel.Hide();
			continue;
		}

		const DirectX::XMFLOAT2 local = SlotLocalPos_(i);
		const float iconX = origin_.x + local.x;
		const float iconY = origin_.y + local.y;
		const float radius = slot.node->GetVisualRadius();
		float infoAnchorY = iconY + radius + kPriceGapBelowIcon;
		if (Canvas2D* price = priceCanvases_[i].get())
		{
			infoAnchorY += static_cast<float>(price->GetCanvasHeight());
		}

		panel.ShowFor(
			slot.node->GetModuleNodeLabel(),
			DirectX::XMFLOAT2{ iconX, infoAnchorY },
			NodeInfoPanel::Anchor::Below,
			kInfoMaxWidthPx,
			false);
	}
	SyncHud();
}

void ModuleShop::EnsurePriceVisuals_()
{
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return;
	}
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		if (priceCanvases_[i] != nullptr)
		{
			continue;
		}
		priceCanvases_[i] = std::make_unique<Canvas2D>(*gfx_, 32u, 16u);
		priceCanvases_[i]->Clear(Colors::None);
		priceCanvases_[i]->LinkTechniques(*rg_);
		paintedPrice_[i] = -1;
	}
}

void ModuleShop::PaintPrice_(std::size_t index)
{
	if (index >= kSlotCount || priceCanvases_[index] == nullptr)
	{
		return;
	}

	const int price = slots_[index].price;
	if (paintedPrice_[index] == price)
	{
		return;
	}

	Canvas2D& canvas = *priceCanvases_[index];
	auto ctx = TextCodex::Get().BeginDraw();
	Text::RenderRequest& rq = ctx.Request();
	rq.text = std::to_string(price);
	rq.canvasMode = Text::CanvasMode::Auto;
	rq.clearMode = Text::ClearMode::Clear;
	rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	rq.fallbackFonts.clear();
	rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
	rq.style.fontSize = kPriceFontSize;
	rq.style.wordWrapEnabled = false;
	rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	rq.maxWidthPx = kSlotPitch;
	rq.paddingPx = 3;
	rq.defaultColor = Color{ 255u, 220u, 90u, 255u };
	rq.backgroundColor = Color{ 24u, 26u, 32u, 220u };
	ctx.Render(canvas);

	const unsigned w = (std::max)(1u, canvas.GetCanvasWidth());
	const unsigned h = (std::max)(1u, canvas.GetCanvasHeight());
	canvas.SetScale(DirectX::XMFLOAT3{
		static_cast<float>(w),
		static_cast<float>(h),
		1.0f
	});
	paintedPrice_[index] = price;
}

void ModuleShop::SyncPriceLabels_()
{
	EnsurePriceVisuals_();
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		Canvas2D* canvas = priceCanvases_[i].get();
		if (canvas == nullptr)
		{
			continue;
		}

		const Slot& slot = slots_[i];
		if (slot.sold || slot.node == nullptr)
		{
			continue;
		}

		PaintPrice_(i);
		const DirectX::XMFLOAT2 local = SlotLocalPos_(i);
		const float radius = slot.node->GetVisualRadius();
		const float halfH = static_cast<float>(canvas->GetCanvasHeight()) * 0.5f;
		canvas->SetPosition(DirectX::XMFLOAT3{
			origin_.x + local.x,
			origin_.y + local.y + radius + kPriceGapBelowIcon + halfH,
			0.0f
		});
	}
}

void ModuleShop::SubmitPrices_()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const Slot& slot = slots_[i];
		if (slot.sold || slot.node == nullptr || priceCanvases_[i] == nullptr)
		{
			continue;
		}
		priceCanvases_[i]->Submit(Chan::ui);
	}
}

void ModuleShop::EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	if (panel_ != nullptr)
	{
		return;
	}

	const float halfX = HalfSpanX_() + kBoundsPad;
	const float halfY = HalfSpanY_() + kBoundsPad;
	const unsigned w = static_cast<unsigned>(std::lround(halfX * 2.0f));
	const unsigned h = static_cast<unsigned>(std::lround(halfY * 2.0f));

	panel_ = std::make_unique<Canvas2D>(gfx, w, h);
	PaintPanel_();
	panel_->LinkTechniques(rg);
	SyncPanelTransform_();
}

void ModuleShop::PaintPanel_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	constexpr Color kBg{ 36u, 48u, 68u, 150u };
	constexpr Color kGrid{ 170u, 190u, 210u, 110u };

	panel_->Clear(kBg);

	const float halfX = HalfSpanX_() + kBoundsPad;
	const float halfY = HalfSpanY_() + kBoundsPad;
	const float localCenterY = HalfSpanY_();
	const float halfPitch = kSlotPitch * 0.5f;

	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const DirectX::XMFLOAT2 slot = SlotLocalPos_(i);
		const float cx = slot.x + halfX;
		const float cy = slot.y - localCenterY + halfY;
		const int x0 = static_cast<int>(std::lround(cx - halfPitch));
		const int y0 = static_cast<int>(std::lround(cy - halfPitch));
		const int x1 = static_cast<int>(std::lround(cx + halfPitch));
		const int y1 = static_cast<int>(std::lround(cy + halfPitch));
		DrawRectOutline(*panel_, x0, y0, x1, y1, kGrid);
	}

	panel_->NotifyPixelsChanged();
}

void ModuleShop::SyncPanelTransform_() noexcept
{
	if (panel_ == nullptr)
	{
		return;
	}

	const BoundsWorld b = GetTradeBoundsWorld();
	panel_->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
	panel_->SetScale(DirectX::XMFLOAT3{
		b.half.x * 2.0f,
		b.half.y * 2.0f,
		1.0f
	});
}

void ModuleShop::InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	EnsureModuleNodeInfoCopyLoaded_();
	EnsurePanelVisual_(gfx, rg);
	for (NodeInfoPanel& panel : infoPanels_)
	{
		panel.Ensure(gfx, rg);
	}
	EnsurePriceVisuals_();
	FillStock();
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->InitVisual(gfx, rg, origin_);
		}
	}
	RelayoutSlots_();
	SyncPanelTransform_();
}

void ModuleShop::SyncZoneTransforms_()
{
	SyncPanelTransform_();
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->SyncVisual();
		}
	}
	SyncInfoPanels_();
}

void ModuleShop::SubmitZoneBackground_()
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

void ModuleShop::SubmitNodes()
{
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->SubmitVisual();
		}
	}
}

void ModuleShop::SubmitInfoPanels()
{
	SubmitPrices_();
	for (NodeInfoPanel& panel : infoPanels_)
	{
		panel.Submit();
	}
	SubmitHud();
}

void ModuleShop::SubmitAllVisuals()
{
	SubmitBackground();
	SubmitNodes();
	SubmitInfoPanels();
}

IModuleNode* ModuleShop::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
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
			origin_.x + local.x,
			origin_.y + local.y
		};
		const Collider2D::CircleCollider hit{ world, node->GetVisualRadius() };
		const Collider2D::PointCollider pt{ worldPos };
		if (!Collider2D::CollisionSystem::IsOverlap(hit, pt))
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