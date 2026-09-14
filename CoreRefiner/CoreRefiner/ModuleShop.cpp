#include "Graphics.h"
#include "RenderGraph.h"
#include "Channels.h"

#include "GameStatsCodex.h"
#include "TextCodex.h"

#include "Collision2D.h"
#include "Colors.h"
#include "IconAtlas.h"

#include "Canvas.h"
#include "CanvasPixelDraw.h"

#include "ModuleShop.h"
#include "ModuleNodeFactory.h"
#include "ModuleNodeLabel.h"
#include "ModuleNodeInfoCopy.h"
#include "UiCopy.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace
{
	struct ShopOffered_
	{
		ModuleNodeLabel data[ModuleNodeLabelCount()]{};
		std::size_t count{ 0 };
	};

	// 可刷表 = Label 全集 − 进化表 − Core 表 − Fusion 表。
	[[nodiscard]] constexpr ShopOffered_ MakeShopOffered_() noexcept
	{
		ShopOffered_ out{};
		for (std::size_t i = 0; i < ModuleNodeLabelCount(); ++i)
		{
			const auto label = static_cast<ModuleNodeLabel>(i);
			if (LabelTableContains(kEvolveLabels, label)
				|| LabelTableContains(kCoreLabels, label)
				|| LabelTableContains(kFusionLabels, label))
			{
				continue;
			}
			out.data[out.count++] = label;
		}
		return out;
	}
	constexpr ShopOffered_ kShopOffered_ = MakeShopOffered_();
	static_assert(kShopOffered_.count >= ModuleShop::kSlotCount);

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

namespace
{
	[[nodiscard]] bool EnsureCanvasSize_(std::unique_ptr<Canvas2D>& canvas, Graphics& gfx, Rgph::RenderGraph& rg, unsigned w, unsigned h)
	{
		if (canvas == nullptr)
		{
			canvas = std::make_unique<Canvas2D>(gfx, w, h);
			canvas->LinkTechniques(rg);
			return true;
		}
		if (canvas->GetCanvasWidth() != w || canvas->GetCanvasHeight() != h)
		{
			canvas->Resize(w, h);
			return true;
		}
		return false;
	}

	void PaintBandChrome_(Canvas2D& canvas)
	{
		constexpr Color kBg{ 36u, 48u, 68u, 150u };
		constexpr Color kFrame{ 170u, 190u, 210u, 90u };
		canvas.Clear(kBg);
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		CanvasPixelDraw::DrawRectOutline(canvas, 0, 0, w - 1, h - 1, kFrame);
	}

	// 货卡造价、顶栏金额、亮起的炼成费用共用。
	constexpr Color kMoneyYellow{ 255u, 220u, 90u, 255u };

	// 商店 UI 共用字体栈：YaHei，回退 Yu Gothic / Segoe。
	void FillUiLabelRequest_(Text::RenderRequest& rq, const std::string& text, float fontSize, Color color)
	{
		rq.text = text;
		rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		rq.fallbackFonts.clear();
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Yu Gothic UI"));
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
		rq.style.fontSize = fontSize;
		rq.defaultColor = color;
	}

	[[nodiscard]] bool PointInAabb_(DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 half, DirectX::XMFLOAT2 pos) noexcept
	{
		return std::fabs(pos.x - center.x) <= half.x
			&& std::fabs(pos.y - center.y) <= half.y;
	}

	// 与 PaintRefinePanel_ 同一套四区等缝。slotXs[0/1/2] = 素材/主体/结果。
	struct RefineLayout_
	{
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

	[[nodiscard]] RefineLayout_ MakeRefineLayout_(int cw, int ch) noexcept
	{
		RefineLayout_ layout{};
		layout.pad = (std::max)(8, ch / 14);
		const int innerW = (std::max)(1, cw - layout.pad * 2);
		const int innerH = (std::max)(1, ch - layout.pad * 2);
		const int regionGapWanted = 64;
		const int regionGapMin = 32;
		layout.btnGap = 5;
		constexpr int slotN = 3;
		constexpr int buttonN = 4;
		layout.btnW = 120;
		layout.btnH = (std::max)(14, (innerH - layout.btnGap * (buttonN - 1)) / buttonN);
		int side = innerH;
		int regionGap = regionGapWanted;
		const int gapN = slotN;
		auto clusterW = [&]() noexcept
		{
			return side * slotN + regionGap * gapN + layout.btnW;
		};
		if (clusterW() > innerW)
		{
			regionGap = (std::max)(regionGapMin, (innerW - side * slotN - layout.btnW) / gapN);
		}
		if (clusterW() > innerW)
		{
			side = (std::max)(24, (innerW - regionGap * gapN - layout.btnW) / slotN);
		}
		if (clusterW() > innerW)
		{
			layout.btnW = (std::max)(72, innerW - side * slotN - regionGap * gapN);
		}
		layout.side = side;
		layout.slotTop = layout.pad + (std::max)(0, innerH - side) / 2;
		const int groupX = layout.pad + (std::max)(0, innerW - clusterW()) / 2;
		layout.slotXs[0] = groupX;
		layout.slotXs[1] = groupX + side + regionGap;
		layout.slotXs[2] = groupX + (side + regionGap) * 2;
		layout.btnX = groupX + (side + regionGap) * slotN;
		const int btnColH = layout.btnH * buttonN + layout.btnGap * (buttonN - 1);
		layout.btnTop = layout.pad + (std::max)(0, innerH - btnColH) / 2;
		return layout;
	}

	// Fixed 用 dest 做排版盒（不再认 maxWidthPx）。价格/描述各自收成一条带。
	void DrawShopText_(
		Canvas2D& canvas,
		const std::string& text,
		float fontSize,
		int boxX,
		int boxY,
		int boxW,
		int boxH,
		int paddingPx,
		DWRITE_TEXT_ALIGNMENT align,
		DWRITE_PARAGRAPH_ALIGNMENT para,
		Color color,
		const std::vector<Text::Span>& spans = {})
	{
		if (text.empty() || boxW <= 0 || boxH <= 0)
		{
			return;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillUiLabelRequest_(rq, text, fontSize, color);
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.style.wordWrapEnabled = true;
		rq.style.textAlign = align;
		rq.style.paragraphAlign = para;
		rq.paddingPx = paddingPx;
		rq.SetDestRect(
			static_cast<float>(boxX),
			static_cast<float>(boxY),
			static_cast<float>(boxW),
			static_cast<float>(boxH));
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
	}

	void DrawShopLabelInBox_(
		Canvas2D& canvas,
		const std::string& text,
		float fontSize,
		int boxX,
		int boxY,
		int boxW,
		int boxH,
		Color color,
		const std::vector<Text::Span>& spans = {})
	{
		if (text.empty() || boxW <= 0 || boxH <= 0)
		{
			return;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillUiLabelRequest_(rq, text, fontSize, color);
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.style.wordWrapEnabled = false;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.paddingPx = 1;
		rq.SetDestRect(
			static_cast<float>(boxX),
			static_cast<float>(boxY),
			static_cast<float>(boxW),
			static_cast<float>(boxH));
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
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

	// 整数倍率贴进矩形中心，不拉长变形。
	void BlitIconInBox_(
		Canvas& dest,
		const IconAtlas::IconBits& bits,
		Color color,
		int boxX,
		int boxY,
		int boxW,
		int boxH)
	{
		if (boxW <= 0 || boxH <= 0)
		{
			return;
		}

		const int scale = (std::max)(1, (std::min)(boxW, boxH) / 16);
		const int glyph = 16 * scale;
		const int ox = boxX + (boxW - glyph) / 2;
		const int oy = boxY + (boxH - glyph) / 2;
		for (unsigned y = 0u; y < 16u; ++y)
		{
			const std::uint16_t row = bits[y];
			for (unsigned x = 0u; x < 16u; ++x)
			{
				if ((row & static_cast<std::uint16_t>(1u << (15u - x))) == 0)
				{
					continue;
				}
				for (int sy = 0; sy < scale; ++sy)
				{
					for (int sx = 0; sx < scale; ++sx)
					{
						CanvasPixelDraw::PutPixelClamped(
							dest,
							ox + static_cast<int>(x) * scale + sx,
							oy + static_cast<int>(y) * scale + sy,
							color);
					}
				}
			}
		}
	}

	void SyncBandTransform_(Canvas2D* canvas, const IModuleZone::BoundsWorld& b)
	{
		if (canvas == nullptr)
		{
			return;
		}
		canvas->SetPosition(DirectX::XMFLOAT3{ b.center.x, b.center.y, 0.0f });
		canvas->SetScale(DirectX::XMFLOAT3{
			b.half.x * 2.0f,
			b.half.y * 2.0f,
			1.0f
		});
	}
}

// ---- 外壳 ----
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
	RelayoutSlots_();
	PlaceRefineResultVisual_();
	SyncTradePanelTransform_();
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncRefineTransform_();
	SyncFunction3Transform_();
}

void ModuleShop::SetShellExtent(float halfX, float halfY) noexcept
{
	shellHalf_.x = (std::max)(1.0f, halfX);
	shellHalf_.y = (std::max)(1.0f, halfY);
	if (gfx_ != nullptr && rg_ != nullptr)
	{
		EnsureTradePanelVisual_(*gfx_, *rg_);
		EnsureRefineVisuals_(*gfx_, *rg_);
		EnsureFunction3Visuals_(*gfx_, *rg_);
	}
	RelayoutSlots_();
	PlaceRefineResultVisual_();
	SyncTradePanelTransform_();
	SyncRefineTransform_();
	SyncFunction3Transform_();
}

void ModuleShop::InitZoneVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	EnsureTradePanelVisual_(gfx, rg);
	EnsureSlotCardVisuals_(gfx, rg);
	EnsureLockButtonVisuals_(gfx, rg);
	EnsureRefineVisuals_(gfx, rg);
	EnsureFunction3Visuals_(gfx, rg);
	FillStock();
	for (Slot& slot : slots_)
	{
		if (slot.node != nullptr)
		{
			slot.node->InitVisual(gfx, rg, origin_);
		}
	}
	RelayoutSlots_();
	SyncTradePanelTransform_();
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncRefineTransform_();
	SyncFunction3Transform_();
}

void ModuleShop::SyncZoneTransforms_()
{
	SyncTradePanelTransform_();
	SyncSlotCardTransforms_();
	SyncLockButtonTransforms_();
	SyncRefineTransform_();
	SyncFunction3Transform_();
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
	if (tradePanel_ != nullptr)
	{
		tradePanel_->Submit(Chan::ui);
	}
	for (auto& card : slotCards_)
	{
		if (card != nullptr)
		{
			card->Submit(Chan::ui);
		}
	}
	if (refinePanel_ != nullptr)
	{
		refinePanel_->Submit(Chan::ui);
	}
	if (function3Panel_ != nullptr)
	{
		function3Panel_->Submit(Chan::ui);
	}
}

// ---- 买卖区 · 逻辑 ----
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
	const float innerW = ContentHalfX_() * 2.0f;
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
	const BoundsWorld refine = GetRefineBoundsWorld_();
	return PointInAabb_(trade.center, trade.half, worldCenter)
		|| PointInAabb_(refine.center, refine.half, worldCenter);
}

void ModuleShop::FillStock()
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
		? slot.node->GetBuyPrice()
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
	const bool up = CanUpgradeRefine_();
	const bool fuse = CanFuseRefine_();
	const bool back = CanReturnRefine_();
	const int cur = GameStatsCodex::GetCurrency();
	if (up != paintedUpgradeLit_ || fuse != paintedFuseLit_ || back != paintedReturnLit_ || cur != paintedRefineCurrency_)
	{
		PaintRefinePanel_();
	}
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
	return PointInAabb_(c, DirectX::XMFLOAT2{ half, half }, worldPos);
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
		if (PointInAabb_(c, DirectX::XMFLOAT2{ half, half }, worldPos))
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
	if (node == nullptr || node->IsCore())
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

	GameStatsCodex::AddCurrency(node->GetSellPrice());
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

	if (refineResult_.get() == &node)
	{
		// 结果格只出不进：炼成板一律 Forbidden；买卖框非 Core 可进货或卖掉。
		const std::size_t refineHit = HitRefineSlotIndex_(worldPos);
		if (refineHit < kRefineSlotCount_)
		{
			result.verdict = DropVerdict::Forbidden;
			return result;
		}
		const BoundsWorld refine = GetRefineBoundsWorld_();
		if (PointInAabb_(refine.center, refine.half, worldPos))
		{
			result.verdict = DropVerdict::Forbidden;
			return result;
		}
		if (node.IsCore())
		{
			result.verdict = DropVerdict::Forbidden;
			return result;
		}
		result.verdict = DropVerdict::Accept;
		return result;
	}

	const std::size_t refineHit = HitRefineSlotIndex_(worldPos);
	if (refineHit < kRefineSlotCount_)
	{
		if (refineHit >= 2u || !CanParkRefine(node, refineHit))
		{
			result.verdict = DropVerdict::Forbidden;
			return result;
		}
		result.verdict = DropVerdict::Accept;
		const DirectX::XMFLOAT2 wc = RefineSlotWorldCenter(refineHit);
		result.localPos = DirectX::XMFLOAT2{ wc.x - origin_.x, wc.y - origin_.y };
		return result;
	}

	const BoundsWorld refine = GetRefineBoundsWorld_();
	if (PointInAabb_(refine.center, refine.half, worldPos))
	{
		result.verdict = DropVerdict::Forbidden;
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
	if (node != nullptr && node == refineResult_.get())
	{
		refineParked_[2] = nullptr;
		std::unique_ptr<IModuleNode> taken = std::move(refineResult_);
		if (taken != nullptr)
		{
			taken->ClearIconRadiusOverride();
			taken->ClearVisualRadiusOverride();
		}
		PaintRefinePanel_();
		return taken;
	}

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
	PaintRefinePanel_();
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
	if (refineResult_ != nullptr)
	{
		refineResult_->SubmitVisual();
	}
}

IModuleNode* ModuleShop::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	for (std::size_t i = 0; i < kRefineSlotCount_; ++i)
	{
		IModuleNode* node = refineParked_[i];
		if (node == nullptr || HitRefineSlotIndex_(worldPos) != i)
		{
			continue;
		}
		const DirectX::XMFLOAT2 wc = RefineSlotWorldCenter(i);
		const float dx = worldPos.x - wc.x;
		const float dy = worldPos.y - wc.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = node;
		}
	}

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

// ---- 买卖区 · 绘制 ----
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
		EnsureCanvasSize_(slotCards_[i], gfx, rg, w, h);
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

	if (slot.sold)
	{
		constexpr Color kSold{ 160u, 160u, 160u, 220u };
		DrawShopText_(
			canvas,
			"SOLD OUT",
			16.0f,
			0,
			0,
			cw,
			ch,
			6,
			DWRITE_TEXT_ALIGNMENT_CENTER,
			DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			kSold);
		canvas.NotifyPixelsChanged();
		return;
	}
	if (slot.node == nullptr)
	{
		canvas.NotifyPixelsChanged();
		return;
	}

	// 价格贴在图标区下沿；描述吃剩余高度，避免整张卡当 layout。
	const int priceTop = iconZoneH;
	const int priceH = static_cast<int>(kPriceFontSize) + 8;
	const int bodyTop = priceTop + priceH;
	const int bodyH = ch - bodyTop;
	DrawShopText_(
		canvas,
		std::to_string(slot.price),
		kPriceFontSize,
		0,
		priceTop,
		cw,
		priceH,
		6,
		DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
		kMoneyYellow);

	const ModuleNodeInfoEntry& entry = GetModuleNodeInfoCopy(slot.node->GetModuleNodeLabel());
	DrawShopText_(
		canvas,
		entry.ComposedText(),
		13.0f,
		0,
		bodyTop,
		cw,
		bodyH,
		6,
		DWRITE_TEXT_ALIGNMENT_LEADING,
		DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
		Colors::White,
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

void ModuleShop::EnsureTradePanelVisual_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, ContentHalfX_() * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, TradeHalfY_() * 2.0f)));
	if (EnsureCanvasSize_(tradePanel_, gfx, rg, w, h))
	{
		PaintTradePanel_();
	}
	SyncTradePanelTransform_();
}

void ModuleShop::PaintTradePanel_()
{
	if (tradePanel_ == nullptr)
	{
		return;
	}

	PaintBandChrome_(*tradePanel_);
	const int w = static_cast<int>(tradePanel_->GetCanvasWidth());
	constexpr Color kHudBar{ 28u, 38u, 56u, 200u };
	constexpr Color kFrame{ 170u, 190u, 210u, 90u };
	const int barBottom = static_cast<int>(std::lround(kHudBarHeight)) - 1;
	for (int y = 1; y < barBottom; ++y)
	{
		CanvasPixelDraw::DrawHLine(*tradePanel_, 1, w - 2, y, kHudBar);
	}
	CanvasPixelDraw::DrawHLine(*tradePanel_, 1, w - 2, barBottom, kFrame);
	tradePanel_->NotifyPixelsChanged();
}

void ModuleShop::SyncTradePanelTransform_() noexcept
{
	SyncBandTransform_(tradePanel_.get(), GetTradeBoundsWorld());
}

// ---- 炼成区 · 逻辑 ----
ModuleShop::BoundsWorld ModuleShop::GetRefineBoundsWorld_() const noexcept
{
	return FunctionBandBounds_(0);
}

bool ModuleShop::RefineWorldToPixel_(DirectX::XMFLOAT2 world, float& px, float& py) const noexcept
{
	const BoundsWorld b = GetRefineBoundsWorld_();
	const float worldW = b.half.x * 2.0f;
	const float worldH = b.half.y * 2.0f;
	if (worldW <= 1.0e-4f || worldH <= 1.0e-4f)
	{
		return false;
	}
	int cw = static_cast<int>(std::lround(worldW));
	int ch = static_cast<int>(std::lround(worldH));
	if (refinePanel_ != nullptr)
	{
		cw = static_cast<int>(refinePanel_->GetCanvasWidth());
		ch = static_cast<int>(refinePanel_->GetCanvasHeight());
	}
	if (cw <= 0 || ch <= 0)
	{
		return false;
	}
	px = (world.x - (b.center.x - b.half.x)) * (static_cast<float>(cw) / worldW);
	py = (world.y - (b.center.y - b.half.y)) * (static_cast<float>(ch) / worldH);
	return true;
}

std::size_t ModuleShop::HitRefineSlotIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	float px = 0.0f;
	float py = 0.0f;
	if (!RefineWorldToPixel_(worldPos, px, py))
	{
		return kRefineSlotCount_;
	}
	int cw = 1;
	int ch = 1;
	if (refinePanel_ != nullptr)
	{
		cw = static_cast<int>(refinePanel_->GetCanvasWidth());
		ch = static_cast<int>(refinePanel_->GetCanvasHeight());
	}
	else
	{
		const BoundsWorld b = GetRefineBoundsWorld_();
		cw = (std::max)(1, static_cast<int>(std::lround(b.half.x * 2.0f)));
		ch = (std::max)(1, static_cast<int>(std::lround(b.half.y * 2.0f)));
	}
	const RefineLayout_ layout = MakeRefineLayout_(cw, ch);
	if (layout.side <= 0)
	{
		return kRefineSlotCount_;
	}
	for (std::size_t i = 0; i < kRefineSlotCount_; ++i)
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
	return kRefineSlotCount_;
}

std::size_t ModuleShop::HitRefineButtonIndex_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	float px = 0.0f;
	float py = 0.0f;
	if (!RefineWorldToPixel_(worldPos, px, py))
	{
		return kRefineButtonCount_;
	}
	int cw = 1;
	int ch = 1;
	if (refinePanel_ != nullptr)
	{
		cw = static_cast<int>(refinePanel_->GetCanvasWidth());
		ch = static_cast<int>(refinePanel_->GetCanvasHeight());
	}
	else
	{
		const BoundsWorld b = GetRefineBoundsWorld_();
		cw = (std::max)(1, static_cast<int>(std::lround(b.half.x * 2.0f)));
		ch = (std::max)(1, static_cast<int>(std::lround(b.half.y * 2.0f)));
	}
	const RefineLayout_ layout = MakeRefineLayout_(cw, ch);
	if (layout.btnW <= 0 || layout.btnH <= 0)
	{
		return kRefineButtonCount_;
	}
	for (std::size_t i = 0; i < kRefineButtonCount_; ++i)
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
	return kRefineButtonCount_;
}

bool ModuleShop::HitRefineButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return HitRefineButtonIndex_(worldPos) < kRefineButtonCount_;
}

std::size_t ModuleShop::HitRefineParkSlot(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const std::size_t i = HitRefineSlotIndex_(worldPos);
	return (i < 2u) ? i : kRefineSlotCount_;
}

DirectX::XMFLOAT2 ModuleShop::RefineSlotWorldCenter(std::size_t slot) const noexcept
{
	const BoundsWorld b = GetRefineBoundsWorld_();
	if (slot >= kRefineSlotCount_)
	{
		return b.center;
	}
	int cw = (std::max)(1, static_cast<int>(std::lround(b.half.x * 2.0f)));
	int ch = (std::max)(1, static_cast<int>(std::lround(b.half.y * 2.0f)));
	if (refinePanel_ != nullptr)
	{
		cw = static_cast<int>(refinePanel_->GetCanvasWidth());
		ch = static_cast<int>(refinePanel_->GetCanvasHeight());
	}
	const RefineLayout_ layout = MakeRefineLayout_(cw, ch);
	const float worldW = b.half.x * 2.0f;
	const float worldH = b.half.y * 2.0f;
	const float px = static_cast<float>(layout.slotXs[slot]) + static_cast<float>(layout.side) * 0.5f;
	const float py = static_cast<float>(layout.slotTop) + static_cast<float>(layout.side) * 0.5f;
	return DirectX::XMFLOAT2{
		b.center.x - b.half.x + px * (worldW / static_cast<float>(cw)),
		b.center.y - b.half.y + py * (worldH / static_cast<float>(ch))
	};
}

float ModuleShop::RefineSlotIconRadius_() const noexcept
{
	const BoundsWorld b = GetRefineBoundsWorld_();
	int cw = (std::max)(1, static_cast<int>(std::lround(b.half.x * 2.0f)));
	int ch = (std::max)(1, static_cast<int>(std::lround(b.half.y * 2.0f)));
	if (refinePanel_ != nullptr)
	{
		cw = static_cast<int>(refinePanel_->GetCanvasWidth());
		ch = static_cast<int>(refinePanel_->GetCanvasHeight());
	}
	const RefineLayout_ layout = MakeRefineLayout_(cw, ch);
	if (layout.side <= 0 || ch <= 0)
	{
		return 8.0f;
	}
	const float worldSide = static_cast<float>(layout.side) * ((b.half.y * 2.0f) / static_cast<float>(ch));
	// 与仓库格相同：半径/格边 = 15/56（kStoredVisualRadius / kSlotPitch）。
	constexpr float kIconToSlot = 15.0f / 56.0f;
	return (std::max)(8.0f, worldSide * kIconToSlot);
}

void ModuleShop::ApplyRefineParkIcon(IModuleNode& node) noexcept
{
	node.SetIconRadiusOverride(RefineSlotIconRadius_());
}

bool ModuleShop::CanParkRefine(const IModuleNode& node, std::size_t slot) const noexcept
{
	if (slot > 1u)
	{
		return false;
	}
	if (refineResult_.get() == &node)
	{
		return false;
	}
	// 买卖框里的商品不能停进炼成格。
	if (FindSlotIndex_(&node) < kSlotCount)
	{
		return false;
	}
	if (node.IsCore() && node.GetLevel() >= ModuleNodeLevel::kMax)
	{
		return false;
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

void ModuleShop::UnbindRefine(IModuleNode* node) noexcept
{
	if (node == nullptr)
	{
		return;
	}
	bool changed = false;
	for (IModuleNode*& slot : refineParked_)
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
		PaintRefinePanel_();
	}
}

bool ModuleShop::IsRefineParked(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return false;
	}
	for (const IModuleNode* slot : refineParked_)
	{
		if (slot == node)
		{
			return true;
		}
	}
	return false;
}

bool ModuleShop::IsRefineResultParked(const IModuleNode* node) const noexcept
{
	return node != nullptr && node == refineResult_.get();
}

void ModuleShop::PlaceRefineResultVisual_()
{
	IModuleNode* node = refineResult_.get();
	if (node == nullptr)
	{
		return;
	}
	const float r = RefineSlotIconRadius_();
	node->SetIconRadiusOverride(r);
	node->SetVisualRadiusOverride(r);
	const DirectX::XMFLOAT2 wc = RefineSlotWorldCenter(2);
	node->SetLocalPos(DirectX::XMFLOAT2{ wc.x - origin_.x, wc.y - origin_.y });
	node->SetZoneOrigin(origin_);
	node->SyncVisual();
}

void ModuleShop::AdoptRefineResult_(std::unique_ptr<IModuleNode> node)
{
	if (node == nullptr)
	{
		return;
	}
	node->EndLayoutGhost();
	node->SetZoneVisualScale(1.0f);
	refineResult_ = std::move(node);
	refineParked_[2] = refineResult_.get();
	PlaceRefineResultVisual_();
	PaintRefinePanel_();
}

void ModuleShop::RestoreRefineResult(std::unique_ptr<IModuleNode> node)
{
	AdoptRefineResult_(std::move(node));
}

void ModuleShop::ClearRefineResult()
{
	refineParked_[2] = nullptr;
	refineResult_.reset();
	PaintRefinePanel_();
}

void ModuleShop::EjectRefineOccupant_(IModuleNode& occupant)
{
	UnbindRefine(&occupant);
	if (occupant.IsLayoutGhostActive())
	{
		occupant.SetLocalPos(occupant.GetCollisionLocalPos());
		occupant.EndLayoutGhost();
	}
	occupant.SyncVisual();
}

void ModuleShop::ParkRefine(std::size_t slot, IModuleNode& node)
{
	if (slot > 1u)
	{
		return;
	}
	UnbindRefine(&node);
	if (IModuleNode* occ = refineParked_[slot])
	{
		if (occ != &node)
		{
			EjectRefineOccupant_(*occ);
		}
	}
	refineParked_[slot] = &node;
	ApplyRefineParkIcon(node);
	PaintRefinePanel_();
}

bool ModuleShop::CanUpgradeRefine_() const noexcept
{
	// 素材等级>=主体、主体<3、结果空、至少 1 元。不看 Label。
	IModuleNode* material = refineParked_[0];
	IModuleNode* subject = refineParked_[1];
	if (material == nullptr || subject == nullptr || refineParked_[2] != nullptr || refineResult_ != nullptr)
	{
		return false;
	}
	if (GameStatsCodex::GetCurrency() < kRefineOpCost_)
	{
		return false;
	}
	if (subject->GetLevel() >= ModuleNodeLevel::kMax)
	{
		return false;
	}
	return material->GetLevel() >= subject->GetLevel();
}

bool ModuleShop::CanFuseRefine_() const noexcept
{
	// 两边都是 3 级、Label 不同、都不是 Fusion、结果空、至少 1 元。Core 不能合成。
	IModuleNode* material = refineParked_[0];
	IModuleNode* subject = refineParked_[1];
	if (material == nullptr || subject == nullptr || refineParked_[2] != nullptr || refineResult_ != nullptr)
	{
		return false;
	}
	if (GameStatsCodex::GetCurrency() < kRefineOpCost_)
	{
		return false;
	}
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

bool ModuleShop::CanReturnRefine_() const noexcept
{
	return refineParked_[0] != nullptr || refineParked_[1] != nullptr;
}

IModuleZone* ModuleShop::FindRefineOwner_(IModuleNode* node, IModuleZone* field, IModuleZone* warehouse) const noexcept
{
	if (node == nullptr)
	{
		return nullptr;
	}
	const std::size_t npos = static_cast<std::size_t>(-1);
	if (field != nullptr && field->FindNodeIndex(node) != npos)
	{
		return field;
	}
	if (warehouse != nullptr && warehouse->FindNodeIndex(node) != npos)
	{
		return warehouse;
	}
	return nullptr;
}

bool ModuleShop::TryUpgradeRefine_(IModuleZone* field, IModuleZone* warehouse)
{
	if (!CanUpgradeRefine_())
	{
		return false;
	}

	IModuleNode* material = refineParked_[0];
	IModuleNode* subject = refineParked_[1];
	IModuleZone* materialOwner = FindRefineOwner_(material, field, warehouse);
	IModuleZone* subjectOwner = FindRefineOwner_(subject, field, warehouse);
	if (material == nullptr || subject == nullptr || materialOwner == nullptr || subjectOwner == nullptr)
	{
		return false;
	}

	const int newLevel = (std::max)(subject->GetLevel() + 1, material->GetLevel());
	const int newPrice = subject->GetBuyPrice() + material->GetBuyPrice();
	if (!GameStatsCodex::TrySpendCurrency(kRefineOpCost_))
	{
		return false;
	}

	UnbindRefine(material);
	UnbindRefine(subject);
	std::unique_ptr<IModuleNode> takenMaterial = materialOwner->TakeNode(material);
	std::unique_ptr<IModuleNode> takenSubject = subjectOwner->TakeNode(subject);
	takenMaterial.reset();
	if (takenSubject == nullptr)
	{
		PaintRefinePanel_();
		return false;
	}

	// 同一颗主体：只改等级/造价，搬进结果格，场/仓不再留影子。
	takenSubject->SetLevel(newLevel);
	takenSubject->SetBuyPrice(newPrice);
	AdoptRefineResult_(std::move(takenSubject));
	materialOwner->SyncAllVisuals();
	if (subjectOwner != materialOwner)
	{
		subjectOwner->SyncAllVisuals();
	}
	return true;
}

bool ModuleShop::TryFuseRefine_(IModuleZone* field, IModuleZone* warehouse)
{
	if (!CanFuseRefine_())
	{
		return false;
	}

	IModuleNode* material = refineParked_[0];
	IModuleNode* subject = refineParked_[1];
	IModuleZone* materialOwner = FindRefineOwner_(material, field, warehouse);
	IModuleZone* subjectOwner = FindRefineOwner_(subject, field, warehouse);
	if (material == nullptr || subject == nullptr || materialOwner == nullptr || subjectOwner == nullptr)
	{
		return false;
	}
	if (gfx_ == nullptr || rg_ == nullptr)
	{
		return false;
	}

	if (!GameStatsCodex::TrySpendCurrency(kRefineOpCost_))
	{
		return false;
	}

	UnbindRefine(material);
	UnbindRefine(subject);
	std::unique_ptr<IModuleNode> takenMaterial = materialOwner->TakeNode(material);
	std::unique_ptr<IModuleNode> takenSubject = subjectOwner->TakeNode(subject);
	if (takenMaterial == nullptr || takenSubject == nullptr)
	{
		PaintRefinePanel_();
		return false;
	}

	takenMaterial->EndLayoutGhost();
	takenSubject->EndLayoutGhost();
	takenMaterial->ClearIconRadiusOverride();
	takenSubject->ClearIconRadiusOverride();
	takenMaterial->ClearVisualRadiusOverride();
	takenSubject->ClearVisualRadiusOverride();

	const DirectX::XMFLOAT2 wc = RefineSlotWorldCenter(2);
	const DirectX::XMFLOAT2 local{ wc.x - origin_.x, wc.y - origin_.y };
	std::unique_ptr<IModuleNode> fusion = ModuleNodeFactory::MakeFusion(
		std::move(takenSubject),
		std::move(takenMaterial),
		local);
	if (fusion == nullptr)
	{
		PaintRefinePanel_();
		return false;
	}

	fusion->InitVisual(*gfx_, *rg_, origin_);
	AdoptRefineResult_(std::move(fusion));
	materialOwner->SyncAllVisuals();
	if (subjectOwner != materialOwner)
	{
		subjectOwner->SyncAllVisuals();
	}
	return true;
}

bool ModuleShop::TryReturnRefine_()
{
	if (!CanReturnRefine_())
	{
		return false;
	}
	IModuleNode* material = refineParked_[0];
	IModuleNode* subject = refineParked_[1];
	if (material != nullptr)
	{
		EjectRefineOccupant_(*material);
	}
	if (subject != nullptr)
	{
		EjectRefineOccupant_(*subject);
	}
	PaintRefinePanel_();
	return true;
}

bool ModuleShop::TryClickRefine(DirectX::XMFLOAT2 worldPos, IModuleZone* field, IModuleZone* warehouse)
{
	const std::size_t i = HitRefineButtonIndex_(worldPos);
	if (i == 0u)
	{
		return TryUpgradeRefine_(field, warehouse);
	}
	if (i == 1u)
	{
		return TryFuseRefine_(field, warehouse);
	}
	if (i == 3u)
	{
		return TryReturnRefine_();
	}
	return false;
}

void ModuleShop::ClearRefineParks()
{
	// 只弹素材/主体；结果格 unique_ptr 留下直到 Reset 或拖出。
	for (std::size_t i = 0; i < 2u; ++i)
	{
		IModuleNode* node = refineParked_[i];
		if (node == nullptr)
		{
			continue;
		}
		refineParked_[i] = nullptr;
		if (node->IsLayoutGhostActive())
		{
			node->SetLocalPos(node->GetCollisionLocalPos());
		}
		node->ClearIconRadiusOverride();
		node->SyncVisual();
	}
	PaintRefinePanel_();
}

// ---- 炼成区 · 绘制 ----
void ModuleShop::EnsureRefineVisuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const BoundsWorld b = GetRefineBoundsWorld_();
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, b.half.x * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, b.half.y * 2.0f)));
	EnsureCanvasSize_(refinePanel_, gfx, rg, w, h);
	PaintRefinePanel_();
	SyncRefineTransform_();
}

void ModuleShop::PaintRefinePanel_()
{
	if (refinePanel_ == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *refinePanel_;
	PaintBandChrome_(canvas);

	for (IModuleNode* node : refineParked_)
	{
		if (node == nullptr)
		{
			continue;
		}
		ApplyRefineParkIcon(*node);
		if (node == refineResult_.get())
		{
			node->SetVisualRadiusOverride(RefineSlotIconRadius_());
		}
	}

	const int cw = static_cast<int>(canvas.GetCanvasWidth());
	const int ch = static_cast<int>(canvas.GetCanvasHeight());
	const RefineLayout_ layout = MakeRefineLayout_(cw, ch);
	const int side = layout.side;
	const int slotTop = layout.slotTop;
	const int* slotXs = layout.slotXs;
	const int regionGap = (side > 0) ? (slotXs[1] - slotXs[0] - side) : 64;
	const int btnW = layout.btnW;
	const int btnH = layout.btnH;
	const int btnX = layout.btnX;
	const int btnTop = layout.btnTop;
	const int btnGap = layout.btnGap;

	constexpr Color kSlotBg{ 42u, 56u, 78u, 190u };
	constexpr Color kSlotFrame{ 170u, 190u, 210u, 150u };
	constexpr Color kSlotText{ 220u, 228u, 236u, 255u };
	constexpr Color kArrow{ 180u, 195u, 215u, 220u };
	constexpr Color kBtnBg{ 48u, 52u, 60u, 220u };
	constexpr Color kBtnFrame{ 96u, 100u, 108u, 200u };
	constexpr Color kBtnText{ 210u, 214u, 220u, 255u };
	constexpr Color kBtnBgLit{ 56u, 92u, 72u, 230u };
	constexpr Color kBtnFrameLit{ 140u, 210u, 170u, 220u };
	constexpr Color kBtnTextLit{ 236u, 244u, 238u, 255u };
	const std::string slotLabels[kRefineSlotCount_] = {
		GetUiCopy("shop.refine.material"),
		GetUiCopy("shop.refine.subject"),
		GetUiCopy("shop.refine.result")
	};
	const UiIconId arrowIds[2] = { UiIconId::RefineFeed, UiIconId::RefineYield };
	const float btnFont = (btnH < 22) ? 11.0f : 13.0f;
	const int btnCorner = (std::max)(4, (std::min)(8, btnH / 2));
	// 素材/主体/结果：直角正方形。右侧四键才圆角。
	for (std::size_t i = 0; i < kRefineSlotCount_; ++i)
	{
		const int x0 = slotXs[i];
		const int y0 = slotTop;
		const int x1 = x0 + side - 1;
		const int y1 = y0 + side - 1;
		CanvasPixelDraw::FillRect(
			canvas,
			static_cast<unsigned>(x0),
			static_cast<unsigned>(y0),
			static_cast<unsigned>(x1),
			static_cast<unsigned>(y1),
			kSlotBg);
		CanvasPixelDraw::DrawRectOutline(canvas, x0, y0, x1, y1, kSlotFrame);
		if (refineParked_[i] != nullptr)
		{
			// 格内有 Node 时不画素材/主体/结果标题，避免压在 Icon 上。
			continue;
		}
		DrawShopLabelInBox_(
			canvas,
			slotLabels[i],
			kRefineSlotLabelFont_,
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

	const std::string btnLabels[kRefineButtonCount_] = {
		GetUiCopy("shop.refine.upgrade"),
		GetUiCopy("shop.refine.fuse"),
		GetUiCopy("shop.refine.evolve"),
		GetUiCopy("shop.refine.return")
	};
	const bool btnLit[kRefineButtonCount_] = {
		CanUpgradeRefine_(),
		CanFuseRefine_(),
		false,
		CanReturnRefine_()
	};
	for (std::size_t i = 0; i < kRefineButtonCount_; ++i)
	{
		const int x0 = btnX;
		const int y0 = btnTop + static_cast<int>(i) * (btnH + btnGap);
		const int x1 = x0 + btnW - 1;
		const int y1 = y0 + btnH - 1;
		const Color bg = btnLit[i] ? kBtnBgLit : kBtnBg;
		const Color frame = btnLit[i] ? kBtnFrameLit : kBtnFrame;
		const Color text = btnLit[i] ? kBtnTextLit : kBtnText;
		CanvasPixelDraw::FillRoundedRect(canvas, x0, y0, x1, y1, btnCorner, bg);
		CanvasPixelDraw::DrawRoundedRectOutline(canvas, x0, y0, x1, y1, btnCorner, frame);
		std::string label = btnLabels[i];
		std::vector<Text::Span> spans;
		if (i < 3u)
		{
			// 升级/合成/进化常驻费用；亮起时只有数字变黄。
			const std::string cost = std::to_string(kRefineOpCost_);
			const UINT32 digitAt = static_cast<UINT32>(
				Utf16CodeUnitCount(label) + Utf16CodeUnitCount(" "));
			label += " " + cost;
			if (btnLit[i])
			{
				Text::Span money{};
				money.start = digitAt;
				money.length = static_cast<UINT32>(Utf16CodeUnitCount(cost));
				money.color = kMoneyYellow;
				spans.push_back(money);
			}
		}
		DrawShopLabelInBox_(
			canvas,
			label,
			btnFont,
			x0,
			y0,
			btnW,
			btnH,
			text,
			spans);
	}

	paintedUpgradeLit_ = btnLit[0];
	paintedFuseLit_ = btnLit[1];
	paintedReturnLit_ = btnLit[3];
	paintedRefineCurrency_ = GameStatsCodex::GetCurrency();
	canvas.NotifyPixelsChanged();
}

void ModuleShop::SyncRefineTransform_() noexcept
{
	SyncBandTransform_(refinePanel_.get(), GetRefineBoundsWorld_());
}

// ---- Function3 · 逻辑 ----
ModuleShop::BoundsWorld ModuleShop::GetFunction3BoundsWorld_() const noexcept
{
	return FunctionBandBounds_(1);
}

// ---- Function3 · 绘制 ----
void ModuleShop::EnsureFunction3Visuals_(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const BoundsWorld b = GetFunction3BoundsWorld_();
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, b.half.x * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, b.half.y * 2.0f)));
	if (EnsureCanvasSize_(function3Panel_, gfx, rg, w, h))
	{
		PaintFunction3Panel_();
	}
	SyncFunction3Transform_();
}

void ModuleShop::PaintFunction3Panel_()
{
	if (function3Panel_ == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *function3Panel_;
	PaintBandChrome_(canvas);
	constexpr Color kTitle{ 180u, 195u, 215u, 200u };
	DrawShopText_(
		canvas,
		"Function3",
		16.0f,
		0,
		0,
		static_cast<int>(canvas.GetCanvasWidth()),
		static_cast<int>(canvas.GetCanvasHeight()),
		4,
		DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
		kTitle);
	canvas.NotifyPixelsChanged();
}

void ModuleShop::SyncFunction3Transform_() noexcept
{
	SyncBandTransform_(function3Panel_.get(), GetFunction3BoundsWorld_());
}