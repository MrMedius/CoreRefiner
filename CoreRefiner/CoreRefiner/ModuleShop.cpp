#include "ModuleShop.h"
#include "CanvasPixelDraw.h"
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
#include <vector>

[[nodiscard]] static bool IsShopOfferedLabel_(ModuleNodeLabel label) noexcept
{
	return label != ModuleNodeLabel::Core_Ball && label < ModuleNodeLabel::Count;
}

[[nodiscard]] static ModuleNodeLabel PickRandomShopLabel_()
{
	static std::mt19937 rng{ std::random_device{}() };
	static std::uniform_int_distribution<int> dist(
		0, static_cast<int>(ModuleNodeLabelCount()) - 1);
	ModuleNodeLabel label = ModuleNodeLabel::Spawn_Ball;
	do
	{
		label = static_cast<ModuleNodeLabel>(dist(rng));
	} while (!IsShopOfferedLabel_(label));
	return label;
}

static_assert(ModuleShop::kSlotCount >= 1);

namespace
{
	void EnsureModuleNodeInfoCopyLoaded_()
	{
		if (IsModuleNodeInfoCopyLoaded())
		{
			return;
		}
		(void)TryLoadCopyWithFallback("ModuleNodeInfoCopy.json", &LoadModuleNodeInfoCopy);
	}

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
		+ static_cast<float>(kFunctionRatio) * static_cast<float>(kReserveCount_);
	return (innerH - 2.0f * kShellInnerGap) / ratioSum;
}

float ModuleShop::TradeHalfX_() const noexcept
{
	return shellHalf_.x - kShellInnerPad;
}

float ModuleShop::TradeHalfY_() const noexcept
{
	return SplitUnitHeight_() * static_cast<float>(kTradeRatio) * 0.5f;
}

float ModuleShop::TradeCenterLocalY_() const noexcept
{
	return -(shellHalf_.y - kShellInnerPad) + TradeHalfY_();
}

float ModuleShop::CardCenterLocalY_() const noexcept
{
	return TradeCenterLocalY_() - TradeHalfY_() + kHudBarHeight + kSlotMargin + kCardHeight * 0.5f;
}

float ModuleShop::SlotPitchX_() const noexcept
{
	const float innerW = TradeHalfX_() * 2.0f;
	return (innerW - 2.0f * kSlotMargin) / static_cast<float>(kSlotCount);
}

DirectX::XMFLOAT2 ModuleShop::SlotLocalPos_(std::size_t index) const noexcept
{
	const float pitch = SlotPitchX_();
	const float x = (static_cast<float>(index) - (static_cast<float>(kSlotCount) - 1.0f) * 0.5f) * pitch;
	const float cardTop = CardCenterLocalY_() - kCardHeight * 0.5f;
	const float y = cardTop + kCardIconPad + kStoredVisualRadius;
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
	const BoundsWorld inner = ShellInnerRect_();
	const float tradeH = SplitUnitHeight_() * static_cast<float>(kTradeRatio);
	BoundsWorld b{};
	b.half = DirectX::XMFLOAT2{
		inner.half.x,
		tradeH * 0.5f
	};
	const float innerTop = inner.center.y - inner.half.y;
	b.center = DirectX::XMFLOAT2{
		inner.center.x,
		innerTop + b.half.y
	};
	return b;
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
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncReserveTransforms_();
}

void ModuleShop::SetShellExtent(float halfX, float halfY) noexcept
{
	shellHalf_.x = (std::max)(1.0f, halfX);
	shellHalf_.y = (std::max)(1.0f, halfY);
	if (gfx_ != nullptr && rg_ != nullptr)
	{
		EnsurePanelVisual_(*gfx_, *rg_);
		EnsureReserveVisuals_(*gfx_, *rg_);
	}
	RelayoutSlots_();
	SyncPanelTransform_();
	SyncReserveTransforms_();
}

void ModuleShop::FillStock()
{
	std::size_t slot = 0;
	for (std::size_t n = 0; slot < kSlotCount && n < ModuleNodeLabelCount() * kSlotCount; ++n)
	{
		const auto label = static_cast<ModuleNodeLabel>(n % ModuleNodeLabelCount());
		if (!IsShopOfferedLabel_(label))
		{
			continue;
		}
		RestockSlot_(slot, label);
		++slot;
	}
	RelayoutSlots_();
}

void ModuleShop::BeginVisit()
{
	ResetVisit();
	RerollStock_();
}

void ModuleShop::RestockSlot_(std::size_t index, ModuleNodeLabel label)
{
	if (index >= kSlotCount)
	{
		return;
	}

	Slot& slot = slots_[index];
	slot.node = ModuleNodeFactory::MakeModuleNode(label, SlotLocalPos_(index));
	slot.sold = false;
	slot.locked = false;
	slot.price = (slot.node != nullptr)
		? ModuleNodePrice::GetBuyPrice(label)
		: 0;

	if (gfx_ != nullptr && rg_ != nullptr && slot.node != nullptr)
	{
		slot.node->InitVisual(*gfx_, *rg_, origin_);
	}
}

bool ModuleShop::HasRerollableSlot_() const noexcept
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

void ModuleShop::DenyRefresh_()
{
	refreshDeniedSec_ = 0.35f;
	PaintHudIcons_();
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

void ModuleShop::RerollStock_()
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const Slot& slot = slots_[i];
		if (slot.locked && !slot.sold && slot.node != nullptr)
		{
			continue;
		}
		RestockSlot_(i, PickRandomShopLabel_());
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
	const BoundsWorld b = GetTradeBoundsWorld();
	const float barY = b.center.y - b.half.y + kHudBarHeight * 0.5f;
	return DirectX::XMFLOAT2{
		b.center.x - b.half.x + kSlotMargin + kHudIconWorld_ * 0.5f,
		barY
	};
}

DirectX::XMFLOAT2 ModuleShop::RefreshButtonCenter_() const noexcept
{
	const BoundsWorld b = GetTradeBoundsWorld();
	const float barY = b.center.y - b.half.y + kHudBarHeight * 0.5f;
	return DirectX::XMFLOAT2{
		b.center.x + b.half.x - kSlotMargin - kHudIconWorld_ * 0.5f,
		barY
	};
}

bool ModuleShop::HitRefreshButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const DirectX::XMFLOAT2 c = RefreshButtonCenter_();
	const float half = kHudIconWorld_ * 0.5f + kHudHitPad_;
	return std::fabs(worldPos.x - c.x) <= half && std::fabs(worldPos.y - c.y) <= half;
}

std::size_t ModuleShop::HitLockSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const float half = kLockButtonWorld_ * 0.5f + kHudHitPad_;
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		const Slot& slot = slots_[i];
		if (slot.sold || slot.node == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 c = LockButtonCenter_(i);
		if (std::fabs(worldPos.x - c.x) <= half && std::fabs(worldPos.y - c.y) <= half)
		{
			return i;
		}
	}
	return kSlotCount;
}

bool ModuleShop::HitLockButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return HitLockSlotIndex_(worldPos) < kSlotCount;
}

bool ModuleShop::ToggleLockAt(DirectX::XMFLOAT2 worldPos)
{
	const std::size_t i = HitLockSlotIndex_(worldPos);
	if (i >= kSlotCount)
	{
		return false;
	}

	slots_[i].locked = !slots_[i].locked;
	PaintSlotCard_(i);
	PaintLockButton_(i);
	return true;
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

void ModuleShop::MarkSold(std::size_t index)
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
	RefreshSlotCards_();
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
	RefreshSlotCards_();
	return taken;
}

void ModuleShop::RefreshCopy()
{
	RefreshSlotCards_();
}

void ModuleShop::RefreshSlotCards_()
{
	if (gfx_ != nullptr && rg_ != nullptr)
	{
		EnsureSlotCardVisuals_(*gfx_, *rg_);
		EnsureLockButtonVisuals_(*gfx_, *rg_);
	}
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		PaintSlotCard_(i);
		PaintLockButton_(i);
	}
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncHud();
}

void ModuleShop::EnsureSlotCardVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned w = (std::max)(1u, static_cast<unsigned>(std::lround(kCardWidth)));
	const unsigned h = (std::max)(1u, static_cast<unsigned>(std::lround(kCardHeight)));
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		if (slotCards_[i] == nullptr)
		{
			slotCards_[i] = std::make_unique<Canvas2D>(gfx, w, h);
			slotCards_[i]->LinkTechniques(rg);
			continue;
		}
		if (slotCards_[i]->GetCanvasWidth() != w || slotCards_[i]->GetCanvasHeight() != h)
		{
			slotCards_[i]->Resize(w, h);
		}
	}
}

void ModuleShop::PaintSlotCard_(std::size_t index)
{
	if (index >= kSlotCount || slotCards_[index] == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *slotCards_[index];
	const Slot& slot = slots_[index];
	const bool empty = slot.sold || slot.node == nullptr;

	constexpr Color kCardBg{ 42u, 56u, 78u, 190u };
	constexpr Color kCardBgLocked{ 22u, 30u, 42u, 230u };
	constexpr Color kEmptyBg{ 28u, 36u, 52u, 150u };
	constexpr Color kFrame{ 170u, 190u, 210u, 150u };
	constexpr Color kFrameLocked{ 88u, 104u, 124u, 210u };
	constexpr Color kPrice{ 255u, 220u, 90u, 255u };

	const bool lockedLook = !empty && slot.locked;
	const Color bg = empty ? kEmptyBg : (lockedLook ? kCardBgLocked : kCardBg);
	const Color frame = lockedLook ? kFrameLocked : kFrame;

	canvas.Clear(bg);
	const int cw = static_cast<int>(canvas.GetCanvasWidth());
	const int ch = static_cast<int>(canvas.GetCanvasHeight());
	CanvasPixelDraw::DrawRectOutline(canvas, 1, 1, cw - 2, ch - 2, frame);

	const int iconZoneH = static_cast<int>(std::lround(
		kCardIconPad + kStoredVisualRadius * 2.0f + kCardIconPad));
	CanvasPixelDraw::DrawHLine(canvas, 8, cw - 9, iconZoneH, frame);

	auto drawText = [&](const std::string& text, float fontSize, float offsetY,
		DWRITE_TEXT_ALIGNMENT align, Color color, bool wrap, const std::vector<Text::Span>& spans,
		DWRITE_PARAGRAPH_ALIGNMENT paraAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
	{
		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = text;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		rq.fallbackFonts.clear();
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Yu Gothic UI"));
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
		rq.style.fontSize = fontSize;
		rq.style.wordWrapEnabled = wrap;
		rq.style.textAlign = align;
		rq.style.paragraphAlign = paraAlign;
		rq.maxWidthPx = static_cast<float>(cw);
		rq.paddingPx = 6;
		rq.drawOffsetYPx = offsetY;
		rq.defaultColor = color;
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
	};

	if (slot.sold)
	{
		constexpr Color kSold{ 160u, 160u, 160u, 220u };
		drawText(
			"SOLD OUT",
			16.0f,
			0.0f,
			DWRITE_TEXT_ALIGNMENT_CENTER,
			kSold,
			true,
			{},
			DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		canvas.NotifyPixelsChanged();
		return;
	}
	if (slot.node == nullptr)
	{
		canvas.NotifyPixelsChanged();
		return;
	}

	drawText(
		std::to_string(slot.price),
		kPriceFontSize,
		static_cast<float>(iconZoneH),
		DWRITE_TEXT_ALIGNMENT_CENTER,
		kPrice,
		true,
		{});

	const ModuleNodeInfoEntry& entry = GetModuleNodeInfoCopy(slot.node->GetModuleNodeLabel());
	drawText(
		entry.ComposedText(),
		13.0f,
		static_cast<float>(iconZoneH) + kPriceFontSize + 8.0f,
		DWRITE_TEXT_ALIGNMENT_LEADING,
		Colors::White,
		true,
		entry.spans);

	canvas.NotifyPixelsChanged();
}

void ModuleShop::SyncSlotCardTransforms_() noexcept
{
	const float cardCenterLocalY = CardCenterLocalY_();
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		Canvas2D* canvas = slotCards_[i].get();
		if (canvas == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 slot = SlotLocalPos_(i);
		canvas->SetPosition(DirectX::XMFLOAT3{
			origin_.x + slot.x,
			origin_.y + cardCenterLocalY,
			0.0f
		});
		canvas->SetScale(DirectX::XMFLOAT3{
			kCardWidth,
			kCardHeight,
			1.0f
		});
	}
}

DirectX::XMFLOAT2 ModuleShop::LockButtonCenter_(std::size_t index) const noexcept
{
	const DirectX::XMFLOAT2 slot = SlotLocalPos_(index);
	const float cardBottom = origin_.y + CardCenterLocalY_() + kCardHeight * 0.5f;
	return DirectX::XMFLOAT2{
		std::round(origin_.x + slot.x),
		std::round(cardBottom + kLockButtonGap_ + kLockButtonWorld_ * 0.5f)
	};
}

void ModuleShop::EnsureLockButtonVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned size = kLockButtonPixels_;
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		if (lockButtons_[i] == nullptr)
		{
			lockButtons_[i] = std::make_unique<Canvas2D>(gfx, size, size);
			lockButtons_[i]->LinkTechniques(rg);
			lockButtons_[i]->SetScale(DirectX::XMFLOAT3{
				kLockButtonWorld_,
				kLockButtonWorld_,
				1.0f
			});
			continue;
		}
		if (lockButtons_[i]->GetCanvasWidth() != size || lockButtons_[i]->GetCanvasHeight() != size)
		{
			lockButtons_[i]->Resize(size, size);
		}
	}
}

void ModuleShop::PaintLockButton_(std::size_t index)
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
	const int cw = static_cast<int>(canvas.GetCanvasWidth());
	const int ch = static_cast<int>(canvas.GetCanvasHeight());
	BlitLockIconCentered_(
		canvas,
		UiIconAtlas::Get(slot.locked ? UiIconId::LockClosed : UiIconId::LockOpen),
		kIcon,
		kLockIconScale_);
	const Color frame = slot.locked ? kBtnFrameLocked : kBtnFrame;
	CanvasPixelDraw::DrawRectBorder(canvas, 0u, 0u, static_cast<unsigned>(cw) - 1u, static_cast<unsigned>(ch) - 1u, kLockBorderTexels_, frame);
	canvas.NotifyPixelsChanged();
}

void ModuleShop::SyncLockButtonTransforms_() noexcept
{
	for (std::size_t i = 0; i < kSlotCount; ++i)
	{
		Canvas2D* canvas = lockButtons_[i].get();
		if (canvas == nullptr)
		{
			continue;
		}
		const DirectX::XMFLOAT2 c = LockButtonCenter_(i);
		canvas->SetPosition(DirectX::XMFLOAT3{ c.x, c.y, 0.0f });
		canvas->SetScale(DirectX::XMFLOAT3{ kLockButtonWorld_, kLockButtonWorld_, 1.0f });
	}
}

void ModuleShop::EnsurePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, TradeHalfX_() * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, TradeHalfY_() * 2.0f)));
	bool painted = false;
	if (panel_ == nullptr)
	{
		panel_ = std::make_unique<Canvas2D>(gfx, w, h);
		panel_->LinkTechniques(rg);
		painted = true;
	}
	else if (panel_->GetCanvasWidth() != w || panel_->GetCanvasHeight() != h)
	{
		panel_->Resize(w, h);
		painted = true;
	}
	if (painted)
	{
		PaintPanel_();
	}
	SyncPanelTransform_();
}

void ModuleShop::PaintPanel_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	constexpr Color kBg{ 36u, 48u, 68u, 150u };
	constexpr Color kFrame{ 170u, 190u, 210u, 90u };

	panel_->Clear(kBg);
	const int w = static_cast<int>(panel_->GetCanvasWidth());
	const int h = static_cast<int>(panel_->GetCanvasHeight());
	constexpr Color kHudBar{ 28u, 38u, 56u, 200u };
	const int barBottom = static_cast<int>(std::lround(kHudBarHeight)) - 1;
	for (int y = 1; y < barBottom; ++y)
	{
		CanvasPixelDraw::DrawHLine(*panel_, 1, w - 2, y, kHudBar);
	}
	CanvasPixelDraw::DrawRectOutline(*panel_, 0, 0, w - 1, h - 1, kFrame);
	CanvasPixelDraw::DrawHLine(*panel_, 1, w - 2, barBottom, kFrame);
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

ModuleShop::BoundsWorld ModuleShop::GetReserveBoundsWorld_(std::size_t index) const noexcept
{
	const BoundsWorld inner = ShellInnerRect_();
	const float unit = SplitUnitHeight_();
	const float tradeH = unit * static_cast<float>(kTradeRatio);
	const float funcH = unit * static_cast<float>(kFunctionRatio);
	const float innerTop = inner.center.y - inner.half.y;
	const float top = innerTop + tradeH + kShellInnerGap
		+ static_cast<float>(index) * (funcH + kShellInnerGap);

	BoundsWorld b{};
	b.half = DirectX::XMFLOAT2{
		inner.half.x,
		funcH * 0.5f
	};
	b.center = DirectX::XMFLOAT2{
		inner.center.x,
		top + b.half.y
	};
	return b;
}

void ModuleShop::EnsureReserveVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const BoundsWorld sample = GetReserveBoundsWorld_(0);
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, sample.half.x * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, sample.half.y * 2.0f)));

	for (std::size_t i = 0; i < kReserveCount_; ++i)
	{
		bool painted = false;
		if (reservePanels_[i] == nullptr)
		{
			reservePanels_[i] = std::make_unique<Canvas2D>(gfx, w, h);
			reservePanels_[i]->LinkTechniques(rg);
			painted = true;
		}
		else if (reservePanels_[i]->GetCanvasWidth() != w || reservePanels_[i]->GetCanvasHeight() != h)
		{
			reservePanels_[i]->Resize(w, h);
			painted = true;
		}
		if (painted)
		{
			PaintReservePanel_(i);
		}
	}
	SyncReserveTransforms_();
}

void ModuleShop::PaintReservePanel_(std::size_t index)
{
	if (index >= kReserveCount_ || reservePanels_[index] == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *reservePanels_[index];
	constexpr Color kBg{ 28u, 36u, 52u, 160u };
	constexpr Color kFrame{ 170u, 190u, 210u, 110u };
	constexpr Color kTitle{ 180u, 195u, 215u, 200u };

	canvas.Clear(kBg);
	const int cw = static_cast<int>(canvas.GetCanvasWidth());
	const int ch = static_cast<int>(canvas.GetCanvasHeight());
	CanvasPixelDraw::DrawRectOutline(canvas, 2, 2, cw - 3, ch - 3, kFrame);

	auto ctx = TextCodex::Get().BeginDraw();
	Text::RenderRequest& rq = ctx.Request();
	rq.text = (index == 0) ? "Function2" : "Function3";
	rq.canvasMode = Text::CanvasMode::Fixed;
	rq.clearMode = Text::ClearMode::NoClear;
	rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	rq.fallbackFonts.clear();
	rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
	rq.style.fontSize = 16.0f;
	rq.style.wordWrapEnabled = true;
	rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
	rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
	rq.maxWidthPx = static_cast<float>(cw);
	rq.paddingPx = 4;
	rq.defaultColor = kTitle;
	rq.backgroundColor = Colors::None;
	ctx.Render(canvas);
	canvas.NotifyPixelsChanged();
}

void ModuleShop::SyncReserveTransforms_() noexcept
{
	for (std::size_t i = 0; i < kReserveCount_; ++i)
	{
		Canvas2D* canvas = reservePanels_[i].get();
		if (canvas == nullptr)
		{
			continue;
		}
		const BoundsWorld b = GetReserveBoundsWorld_(i);
		canvas->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
		canvas->SetScale(DirectX::XMFLOAT3{
			b.half.x * 2.0f,
			b.half.y * 2.0f,
			1.0f
		});
	}
}

void ModuleShop::InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	EnsureModuleNodeInfoCopyLoaded_();
	EnsurePanelVisual_(gfx, rg);
	EnsureSlotCardVisuals_(gfx, rg);
	EnsureLockButtonVisuals_(gfx, rg);
	EnsureReserveVisuals_(gfx, rg);
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
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncReserveTransforms_();
}

void ModuleShop::SyncZoneTransforms_()
{
	SyncPanelTransform_();
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncReserveTransforms_();
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->SyncVisual();
		}
	}
	SyncHud();
}

void ModuleShop::SubmitZoneBackground_()
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
	for (auto& card : slotCards_)
	{
		if (card != nullptr)
		{
			card->Submit(Chan::ui);
		}
	}
	for (auto& canvas : reservePanels_)
	{
		if (canvas != nullptr)
		{
			canvas->Submit(Chan::ui);
		}
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