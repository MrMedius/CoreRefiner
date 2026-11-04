#include "ModuleShopUltra.h"

#include "Channels.h"
#include "GameRulesCodex.h"
#include "GameStatsCodex.h"
#include "ModuleNodeFactory.h"
#include "ModuleNodes.h"
#include "ModuleShopDraw.h"
#include "UiCopy.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace
{
	constexpr int kButtonCount_ = 3;
	// 内部栏位边长 = 奥义栏位 * 3/4。顶边与奥义栏齐平，下面的空档画翻页条。
	constexpr int kInnerNumer_ = 3;
	constexpr int kInnerDenom_ = 4;
	// 翻页条至少要能放下可点的三角。空档更矮时，两栏一起缩小。
	constexpr int kPagerMinH_ = 16;
	constexpr int kPagerTriMax_ = 16;

	[[nodiscard]] const ModuleNode_Ultra* AsUltra_(const IModuleNode* node) noexcept
	{
		return dynamic_cast<const ModuleNode_Ultra*>(node);
	}

	[[nodiscard]] ModuleNode_Ultra* AsUltra_(IModuleNode* node) noexcept
	{
		return dynamic_cast<ModuleNode_Ultra*>(node);
	}

	[[nodiscard]] int InnerSideOf_(int ultraSide) noexcept
	{
		return (std::max)(1, ultraSide * kInnerNumer_ / kInnerDenom_);
	}

	[[nodiscard]] DirectX::XMFLOAT2 ToLocal_(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 world) noexcept
	{
		return DirectX::XMFLOAT2{ world.x - origin.x, world.y - origin.y };
	}

	[[nodiscard]] bool NearLocal_(DirectX::XMFLOAT2 a, DirectX::XMFLOAT2 b) noexcept
	{
		const float dx = a.x - b.x;
		const float dy = a.y - b.y;
		return dx * dx + dy * dy <= 16.0f;
	}

	void PlantInSlot_(IModuleNode& node, DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 local, float radius)
	{
		node.SetLocalPos(local);
		node.SetZoneOrigin(origin);
		node.SetZoneVisualScale(1.0f);
		node.SetVisualRadiusOverride(radius);
		node.SetIconRadiusOverride(radius);
		node.SyncVisual();
	}

	void ClearSlotVisual_(IModuleNode& node)
	{
		node.ClearIconRadiusOverride();
		node.ClearVisualRadiusOverride();
	}

	[[nodiscard]] bool InRect_(float px, float py, int x, int y, int w, int h) noexcept
	{
		return w > 0 && h > 0
			&& px >= static_cast<float>(x)
			&& py >= static_cast<float>(y)
			&& px < static_cast<float>(x + w)
			&& py < static_cast<float>(y + h);
	}

	void DrawSlot_(Canvas2D& canvas, int x, int y, int side)
	{
		if (side <= 1 || x < 0 || y < 0)
		{
			return;
		}
		constexpr Color kSlotBg{ 42u, 56u, 78u, 190u };
		constexpr Color kSlotFrame{ 170u, 190u, 210u, 150u };
		const int x1 = x + side - 1;
		const int y1 = y + side - 1;
		CanvasPixelDraw::FillRect(
			canvas,
			static_cast<unsigned>(x),
			static_cast<unsigned>(y),
			static_cast<unsigned>(x1),
			static_cast<unsigned>(y1),
			kSlotBg);
		CanvasPixelDraw::DrawRectOutline(canvas, x, y, x1, y1, kSlotFrame);
	}

	// cost < 0 表示不标价。亮起时只有价格数字变黄，和炼成按钮一致。
	void DrawActionButton_(
		Canvas2D& canvas,
		int x,
		int y,
		int w,
		int h,
		const std::string& caption,
		int cost,
		bool lit)
	{
		if (w <= 0 || h <= 0)
		{
			return;
		}
		const int x1 = x + w - 1;
		const int y1 = y + h - 1;
		const int corner = (std::max)(4, (std::min)(8, h / 2));
		const Color bg = lit ? ModuleShopDraw::kBtnBgLit : ModuleShopDraw::kBtnBg;
		const Color frame = lit ? ModuleShopDraw::kBtnFrameLit : ModuleShopDraw::kBtnFrame;
		const Color text = lit ? ModuleShopDraw::kBtnTextLit : ModuleShopDraw::kBtnText;
		CanvasPixelDraw::FillRoundedRect(canvas, x, y, x1, y1, corner, bg);
		CanvasPixelDraw::DrawRoundedRectOutline(canvas, x, y, x1, y1, corner, frame);

		std::string label = caption;
		std::vector<Text::Span> spans;
		if (cost >= 0)
		{
			const std::string digits = std::to_string(cost);
			const UINT32 digitAt = static_cast<UINT32>(
				Utf16CodeUnitCount(label) + Utf16CodeUnitCount(" "));
			label += " " + digits;
			if (lit)
			{
				Text::Span money{};
				money.start = digitAt;
				money.length = static_cast<UINT32>(Utf16CodeUnitCount(digits));
				money.color = ModuleShopDraw::kMoneyYellow;
				spans.push_back(money);
			}
		}
		const float font = (h < 22) ? 11.0f : 13.0f;
		ModuleShopDraw::DrawShopLabelInBox_(canvas, label, font, x, y, w, h, text, spans);
	}

	// 翻页条上一组三角和圆点的位置。打孔改变容量后整组重算。
	struct PagerGlyphs_
	{
		bool valid = false;
		int leftX = 0;
		int rightX = 0;
		int triY = 0;
		int triW = 0;
		int triH = 0;
		int count = 0;
		int radius = 0;
		int gap = 0;
		int firstCx = 0;
		int cy = 0;
		int stride = 0;
	};

	[[nodiscard]] int PagerDotDiam_(int radius) noexcept
	{
		return (radius <= 0) ? 1 : (radius * 2 + 1);
	}

	[[nodiscard]] int PagerDotsWidth_(int count, int radius, int gap) noexcept
	{
		if (count <= 0)
		{
			return 0;
		}
		return count * PagerDotDiam_(radius) + (std::max)(0, count - 1) * gap;
	}

	// 左三角、圆点、右三角排成一组，相邻空隙相同，整组在条带里居中。
	[[nodiscard]] PagerGlyphs_ BuildPagerGlyphs_(int pagerX, int pagerY, int pagerW, int pagerH, int capacity) noexcept
	{
		PagerGlyphs_ glyphs{};
		if (capacity <= 0 || pagerW < 8 || pagerH < 6)
		{
			return glyphs;
		}
		const int triH = (std::min)(kPagerTriMax_, (std::max)(1, pagerH - 4));
		const int triW = (std::max)(8, triH);
		if (triW * 2 >= pagerW)
		{
			return glyphs;
		}

		int radius = (std::min)(4, (std::max)(2, triH / 5));
		int gap = (std::max)(3, radius);
		auto groupWidth = [&](int r, int g) noexcept
		{
			return triW + g + PagerDotsWidth_(capacity, r, g) + g + triW;
		};
		for (;;)
		{
			if (groupWidth(radius, gap) <= pagerW || (radius <= 0 && gap <= 0))
			{
				break;
			}
			if (gap > 1)
			{
				--gap;
			}
			else if (radius > 1)
			{
				--radius;
			}
			else if (gap > 0)
			{
				--gap;
			}
			else if (radius > 0)
			{
				--radius;
			}
			else
			{
				break;
			}
		}

		glyphs.valid = true;
		glyphs.count = capacity;
		glyphs.triW = triW;
		glyphs.triH = triH;
		glyphs.triY = pagerY + (std::max)(0, pagerH - triH) / 2;
		glyphs.cy = glyphs.triY + (triH - 1) / 2;
		glyphs.radius = radius;
		glyphs.gap = gap;

		const int used = groupWidth(radius, gap);
		if (used <= pagerW)
		{
			const int origin = pagerX + (pagerW - used) / 2;
			glyphs.leftX = origin;
			const int dotsLeft = origin + triW + gap;
			glyphs.firstCx = dotsLeft + ((radius <= 0) ? 0 : radius);
			glyphs.stride = PagerDotDiam_(radius) + gap;
			glyphs.rightX = dotsLeft + PagerDotsWidth_(capacity, radius, gap) + gap;
			return glyphs;
		}

		// 缩到极限仍超宽时，三角留在条带内，点挤在两条三角中间。
		glyphs.leftX = pagerX;
		glyphs.rightX = pagerX + pagerW - triW;
		glyphs.radius = 0;
		glyphs.gap = 0;
		const int innerL = glyphs.leftX + triW;
		const int innerR = glyphs.rightX;
		if (capacity <= 1)
		{
			glyphs.firstCx = (innerL + innerR) / 2;
			glyphs.stride = 0;
		}
		else
		{
			const int span = (std::max)(0, innerR - innerL - 1);
			glyphs.stride = span / (capacity - 1);
			glyphs.firstCx = innerL;
		}
		return glyphs;
	}

	// 翻页三角。顶点交给 CanvasPixelDraw，颜色用商店按钮的亮灭。
	void DrawPagerArrow_(Canvas2D& canvas, int x, int y, int w, int h, bool pointRight, bool lit)
	{
		if (w <= 1 || h <= 1)
		{
			return;
		}
		const Color fill = lit ? ModuleShopDraw::kBtnBgLit : ModuleShopDraw::kBtnBg;
		const Color frame = lit ? ModuleShopDraw::kBtnFrameLit : ModuleShopDraw::kBtnFrame;
		const int midY = y + (h - 1) / 2;
		const int baseX = pointRight ? x : (x + w - 1);
		const int tipX = pointRight ? (x + w - 1) : x;
		CanvasPixelDraw::FillTriangle(canvas, tipX, midY, baseX, y, baseX, y + h - 1, fill);
		CanvasPixelDraw::DrawTriangleOutline(canvas, tipX, midY, baseX, y, baseX, y + h - 1, frame);
	}

	// 翻页圆点。实心先填再描边，空心只描灭按钮的灰边。
	void DrawPagerDot_(Canvas2D& canvas, int cx, int cy, int radius, bool filled)
	{
		const Color fill = ModuleShopDraw::kBtnBgLit;
		const Color frame = filled ? ModuleShopDraw::kBtnFrameLit : ModuleShopDraw::kBtnFrame;
		if (radius <= 0)
		{
			CanvasPixelDraw::FillCircle(canvas, cx, cy, 0, filled ? fill : frame);
			return;
		}
		if (filled)
		{
			CanvasPixelDraw::FillCircle(canvas, cx, cy, radius, fill);
		}
		CanvasPixelDraw::DrawCircleOutline(canvas, cx, cy, radius, frame);
	}
}

void ModuleShopUltra::SetBounds(IModuleZone::BoundsWorld bounds) noexcept
{
	bounds_ = bounds;
}

	// 右侧按钮左边能放下几格，不按容量截断。多出来的格子靠滚动看。
	int ModuleShopUltra::FitInnerSlots_(const Layout& layout) noexcept
	{
		if (layout.innerSide <= 0)
		{
			return 0;
		}
		const int limit = layout.btnX - layout.innerGap;
		const int stride = layout.innerSide + layout.innerGap;
		if (stride <= 0 || layout.innerX0 > limit)
		{
			return 0;
		}
		int count = 0;
		int x = layout.innerX0;
		while (x + layout.innerSide <= limit)
		{
			++count;
			x += stride;
		}
		return count;
	}

void ModuleShopUltra::FillLayout_(Layout& layout) noexcept
{
	layout.pad = (std::max)(6, layout.ch / 16);
	const int innerW = (std::max)(1, layout.cw - layout.pad * 2);
	const int innerH = (std::max)(1, layout.ch - layout.pad * 2);

	layout.btnGap = 4;
	layout.btnW = (std::min)(120, (std::max)(72, innerW / 5));
	layout.btnH = (std::max)(14, (innerH - layout.btnGap * (kButtonCount_ - 1)) / kButtonCount_);
	layout.innerGap = (std::max)(4, layout.pad / 2);

	int ultraSide = innerH;
	int innerSide = InnerSideOf_(ultraSide);
	// 顶对齐后，内部栏下面剩的高度就是翻页条。不够一条时，两栏一起缩小。
	if (layout.capacity > 0 && innerH - innerSide < kPagerMinH_)
	{
		const int innerMax = (std::max)(1, innerH - kPagerMinH_);
		ultraSide = innerMax * kInnerDenom_ / kInnerNumer_;
		ultraSide = (std::min)((std::max)(8, ultraSide), innerH);
		innerSide = InnerSideOf_(ultraSide);
		while (ultraSide > 8 && innerH - innerSide < kPagerMinH_)
		{
			--ultraSide;
			innerSide = InnerSideOf_(ultraSide);
		}
	}
	// 窄到连一格都放不下时，奥义栏和内部栏按 4:3 一起缩小。
	if (layout.capacity > 0)
	{
		const int room = innerW - layout.btnW - layout.innerGap * 2;
		if (ultraSide + innerSide > room && room > 8)
		{
			const int fitted = (std::min)(
				ultraSide,
				(std::max)(24, room * kInnerDenom_ / (kInnerDenom_ + kInnerNumer_)));
			ultraSide = (std::min)(fitted, innerH);
			innerSide = InnerSideOf_(ultraSide);
		}
	}

	layout.ultraSide = (std::max)(1, (std::min)(ultraSide, innerH));
	layout.innerSide = InnerSideOf_(layout.ultraSide);
	layout.ultraX = layout.pad;
	layout.ultraY = layout.pad;
	layout.btnX = layout.cw - layout.pad - layout.btnW;
	const int maxUltra = layout.btnX - layout.innerGap - layout.ultraX;
	if (maxUltra > 0 && layout.ultraSide > maxUltra)
	{
		layout.ultraSide = maxUltra;
		layout.innerSide = InnerSideOf_(layout.ultraSide);
	}
	layout.innerX0 = layout.ultraX + layout.ultraSide + layout.innerGap;
	layout.innerY = layout.ultraY;

	const int colH = layout.btnH * kButtonCount_ + layout.btnGap * (kButtonCount_ - 1);
	layout.btnY = layout.pad + (std::max)(0, innerH - colH) / 2;
	layout.windowCount = FitInnerSlots_(layout);
	layout.visibleBegin = 0;
	layout.visibleCount = 0;

	layout.forgeW = (std::min)(168, (std::max)(72, layout.cw / 4));
	layout.forgeH = (std::min)(40, (std::max)(18, layout.ch / 3));
	layout.forgeW = (std::min)(layout.forgeW, (std::max)(1, layout.cw - 8));
	layout.forgeH = (std::min)(layout.forgeH, (std::max)(1, layout.ch - 8));
	layout.forgeX = (std::max)(0, (layout.cw - layout.forgeW) / 2);
	layout.forgeY = (std::max)(0, (layout.ch - layout.forgeH) / 2);

	const int contentBottom = layout.pad + innerH;
	layout.pagerX = layout.innerX0;
	layout.pagerY = layout.innerY + layout.innerSide;
	layout.pagerW = (std::max)(0, layout.btnX - layout.innerGap - layout.pagerX);
	layout.pagerH = (std::max)(0, contentBottom - layout.pagerY);
}

DirectX::XMFLOAT2 ModuleShopUltra::PixelCenterToWorld_(const Layout& layout, float px, float py) noexcept
{
	const float worldW = layout.bounds.half.x * 2.0f;
	const float worldH = layout.bounds.half.y * 2.0f;
	if (layout.cw <= 0 || layout.ch <= 0 || worldW <= 1.0e-4f || worldH <= 1.0e-4f)
	{
		return layout.bounds.center;
	}
	return DirectX::XMFLOAT2{
		layout.bounds.center.x - layout.bounds.half.x + px * (worldW / static_cast<float>(layout.cw)),
		layout.bounds.center.y - layout.bounds.half.y + py * (worldH / static_cast<float>(layout.ch))
	};
}

bool ModuleShopUltra::WorldToPixel_(const Layout& layout, DirectX::XMFLOAT2 world, float& px, float& py) noexcept
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

// 与仓库格、炼成格相同：半径/格边 = 15/56。
float ModuleShopUltra::SlotRadius_(const Layout& layout, int side) noexcept
{
	if (side <= 0 || layout.ch <= 0)
	{
		return 8.0f;
	}
	const float worldSide = static_cast<float>(side) * ((layout.bounds.half.y * 2.0f) / static_cast<float>(layout.ch));
	constexpr float kIconToSlot = 15.0f / 56.0f;
	return (std::max)(8.0f, worldSide * kIconToSlot);
}

ModuleShopUltra::Layout ModuleShopUltra::BuildLayout_() const noexcept
{
	Layout layout{};
	layout.bounds = bounds_;
	layout.cw = (std::max)(1, static_cast<int>(std::lround(bounds_.half.x * 2.0f)));
	layout.ch = (std::max)(1, static_cast<int>(std::lround(bounds_.half.y * 2.0f)));
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
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	layout.capacity = (ultra != nullptr) ? ultra->GetCapacity() : 0;
	FillLayout_(layout);
	const int begin = ClampedScroll_(layout);
	layout.visibleBegin = begin;
	const int remain = layout.capacity - begin;
	layout.visibleCount = (remain > 0) ? (std::min)(layout.windowCount, remain) : 0;
	return layout;
}

int ModuleShopUltra::ClampedScroll_(const Layout& layout) const noexcept
{
	const int maxBegin = (std::max)(0, layout.capacity - layout.windowCount);
	int begin = scroll_;
	if (begin < 0)
	{
		begin = 0;
	}
	if (begin > maxBegin)
	{
		begin = maxBegin;
	}
	return begin;
}

bool ModuleShopUltra::NeedsPaint_() const noexcept
{
	if (panel_ == nullptr || !painted_)
	{
		return true;
	}
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	const bool has = ultra_ != nullptr;
	const int capacity = (ultra != nullptr) ? ultra->GetCapacity() : 0;
	const std::uint32_t revision = (ultra != nullptr) ? ultra->GetContentRevision() : 0u;
	return has != paintedHas_
		|| capacity != paintedCapacity_
		|| revision != paintedRevision_
		|| scroll_ != paintedScroll_
		|| GameStatsCodex::GetCurrency() != paintedCurrency_;
}

void ModuleShopUltra::Ensure(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	const unsigned w = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.x * 2.0f)));
	const unsigned h = static_cast<unsigned>(std::lround((std::max)(1.0f, bounds_.half.y * 2.0f)));
	const bool resized = ModuleShopDraw::EnsureCanvasSize_(panel_, gfx, rg, w, h);
	if (resized || NeedsPaint_())
	{
		Paint_();
	}
	Sync();
}

void ModuleShopUltra::Sync()
{
	if (NeedsPaint_())
	{
		Paint_();
	}
	ModuleShopDraw::SyncBandTransform_(panel_.get(), bounds_);
	RealignVisible_();
}

void ModuleShopUltra::Submit() const
{
	if (panel_ != nullptr)
	{
		panel_->Submit(Chan::ui);
	}
}

void ModuleShopUltra::Adopt(std::unique_ptr<IModuleNode> node)
{
	ultra_ = std::move(node);
	painted_ = false;
	if (panel_ != nullptr)
	{
		Paint_();
	}
	RealignVisible_();
}

std::unique_ptr<IModuleNode> ModuleShopUltra::Release()
{
	painted_ = false;
	std::unique_ptr<IModuleNode> out = std::move(ultra_);
	if (panel_ != nullptr)
	{
		Paint_();
	}
	return out;
}

void ModuleShopUltra::ClearForRunEnd()
{
	scroll_ = 0;
	std::unique_ptr<IModuleNode> dropped = Release();
	dropped.reset();
}

IModuleNode* ModuleShopUltra::GetUltra() noexcept
{
	return ultra_.get();
}

const IModuleNode* ModuleShopUltra::GetUltra() const noexcept
{
	return ultra_.get();
}

bool ModuleShopUltra::HasUltra() const noexcept
{
	return ultra_ != nullptr;
}

ModuleShopUltra::Button ModuleShopUltra::HitButton(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = BuildLayout_();
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(layout, worldPos, px, py))
	{
		return Button::None;
	}
	if (ultra_ == nullptr)
	{
		return InRect_(px, py, layout.forgeX, layout.forgeY, layout.forgeW, layout.forgeH)
			? Button::Forge
			: Button::None;
	}
	const Button buttons[kButtonCount_] = { Button::Punch, Button::Rename, Button::Redraw };
	for (int i = 0; i < kButtonCount_; ++i)
	{
		const int y = layout.btnY + i * (layout.btnH + layout.btnGap);
		if (InRect_(px, py, layout.btnX, y, layout.btnW, layout.btnH))
		{
			return buttons[i];
		}
	}
	const PagerGlyphs_ glyphs = BuildPagerGlyphs_(
		layout.pagerX,
		layout.pagerY,
		layout.pagerW,
		layout.pagerH,
		layout.capacity);
	if (!glyphs.valid)
	{
		return Button::None;
	}
	if (InRect_(px, py, glyphs.leftX, glyphs.triY, glyphs.triW, glyphs.triH))
	{
		return Button::PageLeft;
	}
	if (InRect_(px, py, glyphs.rightX, glyphs.triY, glyphs.triW, glyphs.triH))
	{
		return Button::PageRight;
	}
	return Button::None;
}

bool ModuleShopUltra::TryClick(DirectX::XMFLOAT2 worldPos, DirectX::XMFLOAT3 zoneOrigin)
{
	switch (HitButton(worldPos))
	{
	case Button::Forge:
		return TryForge(zoneOrigin);
	case Button::Punch:
		return TryPunch();
	case Button::PageLeft:
		return TryPage_(-1);
	case Button::PageRight:
		return TryPage_(1);
	default:
		return false;
	}
}

bool ModuleShopUltra::TryPage_(int direction)
{
	if (direction == 0 || ultra_ == nullptr)
	{
		return false;
	}
	const Layout layout = BuildLayout_();
	const int current = ClampedScroll_(layout);
	const int maxBegin = (std::max)(0, layout.capacity - layout.windowCount);
	int next = current + ((direction < 0) ? -1 : 1);
	if (next < 0)
	{
		next = 0;
	}
	if (next > maxBegin)
	{
		next = maxBegin;
	}
	if (next == current)
	{
		return false;
	}
	scroll_ = next;
	if (panel_ != nullptr)
	{
		Paint_();
	}
	RealignVisible_();
	return true;
}

bool ModuleShopUltra::TryForge(DirectX::XMFLOAT3 zoneOrigin)
{
	if (ultra_ != nullptr || gfx_ == nullptr || rg_ == nullptr)
	{
		return false;
	}
	if (GameStatsCodex::GetCurrency() < GameRulesCodex::ForgeCost)
	{
		return false;
	}

	const DirectX::XMFLOAT2 center = GetUltraSlotCenter();
	const float radius = GetUltraSlotRadius();
	std::unique_ptr<IModuleNode> node = ModuleNodeFactory::MakeUltra(DirectX::XMFLOAT2{
		center.x - zoneOrigin.x,
		center.y - zoneOrigin.y
	});
	if (node == nullptr)
	{
		return false;
	}
	node->InitVisual(*gfx_, *rg_, zoneOrigin);
	node->SetZoneVisualScale(1.0f);
	node->SetVisualRadiusOverride(radius);
	node->SetIconRadiusOverride(radius);
	node->SyncVisual();
	if (!GameStatsCodex::TrySpendCurrency(GameRulesCodex::ForgeCost))
	{
		return false;
	}
	Adopt(std::move(node));
	return true;
}

bool ModuleShopUltra::TryPunch()
{
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	if (ultra == nullptr || GameStatsCodex::GetCurrency() < GameRulesCodex::PunchCost)
	{
		return false;
	}
	if (!GameStatsCodex::TrySpendCurrency(GameRulesCodex::PunchCost))
	{
		return false;
	}
	ultra->AddCapacity();
	if (panel_ != nullptr)
	{
		Paint_();
	}
	RealignVisible_();
	return true;
}

const IModuleNode* ModuleShopUltra::PickInfoNode(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const IModuleNode* ultra = GetUltra();
	if (ultra == nullptr)
	{
		return nullptr;
	}
	const DirectX::XMFLOAT2 center = GetUltraSlotCenter();
	const float radius = GetUltraSlotRadius();
	const float dx = worldPos.x - center.x;
	const float dy = worldPos.y - center.y;
	if (dx * dx + dy * dy > radius * radius)
	{
		return nullptr;
	}
	return ultra;
}

bool ModuleShopUltra::InBand_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	return ModuleShopDraw::PointInAabb_(bounds_.center, bounds_.half, worldPos);
}

bool ModuleShopUltra::HitUltraSlot_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = BuildLayout_();
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(layout, worldPos, px, py))
	{
		return false;
	}
	return InRect_(px, py, layout.ultraX, layout.ultraY, layout.ultraSide, layout.ultraSide);
}

int ModuleShopUltra::HitVisibleInnerSlot_(DirectX::XMFLOAT2 worldPos) const noexcept
{
	const Layout layout = BuildLayout_();
	if (ultra_ == nullptr || layout.visibleCount <= 0)
	{
		return -1;
	}
	float px = 0.0f;
	float py = 0.0f;
	if (!WorldToPixel_(layout, worldPos, px, py))
	{
		return -1;
	}
	const int stride = layout.innerSide + layout.innerGap;
	for (int i = 0; i < layout.visibleCount; ++i)
	{
		const int x = layout.innerX0 + i * stride;
		if (InRect_(px, py, x, layout.innerY, layout.innerSide, layout.innerSide))
		{
			return layout.visibleBegin + i;
		}
	}
	return -1;
}

bool ModuleShopUltra::Owns(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return false;
	}
	if (ultra_.get() == node)
	{
		return true;
	}
	const int count = GetInnerSlotCount();
	for (int i = 0; i < count; ++i)
	{
		if (GetInnerSlot(static_cast<std::size_t>(i)) == node)
		{
			return true;
		}
	}
	return false;
}

DropResult ModuleShopUltra::EvalDrop(const IModuleNode& node, DirectX::XMFLOAT2 worldPos, DirectX::XMFLOAT3 zoneOrigin) const noexcept
{
	DropResult result{};
	result.verdict = DropVerdict::Forbidden;
	// 正在拖这颗奥义时，它还占着栏位，但整块奥义区仍应能把它放回去。
	if (!HasUltra() || ultra_.get() == &node)
	{
		// 没奥义时整块奥义区都能接，松手后仍落在左侧栏位中心。
		if (node.GetKind() == ModuleNodeKind::Ultra && InBand_(worldPos))
		{
			result.verdict = DropVerdict::Accept;
			result.localPos = ToLocal_(zoneOrigin, GetUltraSlotCenter());
			return result;
		}
		if (!HasUltra())
		{
			return result;
		}
	}
	if (HitUltraSlot_(worldPos))
	{
		// 已有奥义时，只有画出来的奥义栏讨论另一颗奥义，并且拒绝。
		return result;
	}

	const int inner = HitVisibleInnerSlot_(worldPos);
	if (inner < 0)
	{
		return result;
	}
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	const IModuleNode* held = GetInnerSlot(static_cast<std::size_t>(inner));
	// 正在从这一格拖走的 Node 不算占用，可以放回原格。
	if (ultra == nullptr || !ultra->CanHold(node) || (held != nullptr && held != &node))
	{
		return result;
	}
	result.verdict = DropVerdict::Accept;
	const DirectX::XMFLOAT2 center = GetInnerSlotCenter(static_cast<std::size_t>(inner));
	result.localPos = ToLocal_(zoneOrigin, center);
	return result;
}

ModuleShopUltra::DropClaim ModuleShopUltra::TryAcceptDrop(
	std::unique_ptr<IModuleNode>& node,
	DirectX::XMFLOAT2 localPos,
	DirectX::XMFLOAT3 zoneOrigin)
{
	if (node == nullptr)
	{
		return DropClaim::Ignore;
	}

	const DirectX::XMFLOAT2 ultraLocal = ToLocal_(zoneOrigin, GetUltraSlotCenter());
	if (NearLocal_(localPos, ultraLocal))
	{
		if (HasUltra() || node->GetKind() != ModuleNodeKind::Ultra)
		{
			return DropClaim::Reject;
		}
		PlantInSlot_(*node, zoneOrigin, ultraLocal, GetUltraSlotRadius());
		Adopt(std::move(node));
		return DropClaim::Accept;
	}

	const int count = GetInnerSlotCount();
	for (int i = 0; i < count; ++i)
	{
		const std::size_t index = static_cast<std::size_t>(i);
		if (!IsInnerSlotVisible(index))
		{
			continue;
		}
		const DirectX::XMFLOAT2 slotLocal = ToLocal_(zoneOrigin, GetInnerSlotCenter(index));
		if (!NearLocal_(localPos, slotLocal))
		{
			continue;
		}
		ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
		if (ultra == nullptr || !ultra->TryPut(index, node))
		{
			return DropClaim::Reject;
		}
		if (IModuleNode* placed = GetInnerSlot(index))
		{
			PlantInSlot_(*placed, zoneOrigin, slotLocal, GetInnerSlotRadius());
		}
		if (panel_ != nullptr)
		{
			Paint_();
		}
		return DropClaim::Accept;
	}
	return DropClaim::Ignore;
}

std::unique_ptr<IModuleNode> ModuleShopUltra::TakeNode(IModuleNode* node)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	if (ultra_.get() == node)
	{
		std::unique_ptr<IModuleNode> taken = Release();
		if (taken != nullptr)
		{
			ClearSlotVisual_(*taken);
		}
		return taken;
	}
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	if (ultra == nullptr)
	{
		return nullptr;
	}
	std::unique_ptr<IModuleNode> taken = ultra->Take(node);
	if (taken == nullptr)
	{
		return nullptr;
	}
	ClearSlotVisual_(*taken);
	if (panel_ != nullptr)
	{
		Paint_();
	}
	return taken;
}

void ModuleShopUltra::PinHomeGhost(IModuleNode& node, DirectX::XMFLOAT3 zoneOrigin) noexcept
{
	DirectX::XMFLOAT2 center = GetUltraSlotCenter();
	float radius = GetUltraSlotRadius();
	if (ultra_.get() != &node)
	{
		const std::size_t index = FindInnerIndex(&node);
		if (index == static_cast<std::size_t>(-1))
		{
			return;
		}
		center = GetInnerSlotCenter(index);
		radius = GetInnerSlotRadius();
	}
	const DirectX::XMFLOAT2 local = ToLocal_(zoneOrigin, center);
	node.SetZoneOrigin(zoneOrigin);
	node.SetLocalPos(local);
	node.BeginLayoutGhost(local);
	node.SetVisualRadiusOverride(radius);
	node.SyncVisual();
}

void ModuleShopUltra::Restore(std::unique_ptr<IModuleNode> node, DirectX::XMFLOAT3 zoneOrigin)
{
	if (node == nullptr || node->GetKind() != ModuleNodeKind::Ultra || HasUltra())
	{
		return;
	}
	node->EndLayoutGhost();
	PlantInSlot_(*node, zoneOrigin, ToLocal_(zoneOrigin, GetUltraSlotCenter()), GetUltraSlotRadius());
	Adopt(std::move(node));
}

void ModuleShopUltra::SetHostOrigin(DirectX::XMFLOAT3 origin) noexcept
{
	hostOrigin_ = origin;
}

std::size_t ModuleShopUltra::FindInnerIndex(const IModuleNode* node) const noexcept
{
	if (node == nullptr)
	{
		return static_cast<std::size_t>(-1);
	}
	const int count = GetInnerSlotCount();
	for (int i = 0; i < count; ++i)
	{
		if (GetInnerSlot(static_cast<std::size_t>(i)) == node)
		{
			return static_cast<std::size_t>(i);
		}
	}
	return static_cast<std::size_t>(-1);
}

void ModuleShopUltra::RestoreInner(std::unique_ptr<IModuleNode> node, std::size_t index, DirectX::XMFLOAT3 zoneOrigin)
{
	if (node == nullptr)
	{
		return;
	}
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	node->EndLayoutGhost();
	if (ultra == nullptr || !ultra->TryPut(index, node))
	{
		return;
	}
	if (IModuleNode* placed = GetInnerSlot(index))
	{
		PlantInSlot_(*placed, zoneOrigin, ToLocal_(zoneOrigin, GetInnerSlotCenter(index)), GetInnerSlotRadius());
	}
	if (panel_ != nullptr)
	{
		Paint_();
	}
}

void ModuleShopUltra::RealignVisible_()
{
	const int count = GetInnerSlotCount();
	if (count <= 0)
	{
		return;
	}
	const float radius = GetInnerSlotRadius();
	for (int i = 0; i < count; ++i)
	{
		const std::size_t index = static_cast<std::size_t>(i);
		IModuleNode* node = GetInnerSlot(index);
		if (node == nullptr || node->IsLayoutGhostActive() || !IsInnerSlotVisible(index))
		{
			continue;
		}
		if (!node->IsVisualReady() && gfx_ != nullptr && rg_ != nullptr)
		{
			node->InitVisual(*gfx_, *rg_, hostOrigin_);
		}
		PlantInSlot_(*node, hostOrigin_, ToLocal_(hostOrigin_, GetInnerSlotCenter(index)), radius);
	}
}

IModuleNode* ModuleShopUltra::PickAt(DirectX::XMFLOAT2 worldPos, float& outDistSq) noexcept
{
	IModuleNode* best = nullptr;
	float bestDistSq = 1.0e9f;

	// 点选范围是画出来的整格奥义栏，空奥义也能从这一格拖出来。
	if (IModuleNode* body = GetUltra(); body != nullptr && HitUltraSlot_(worldPos))
	{
		const DirectX::XMFLOAT2 center = GetUltraSlotCenter();
		const float dx = worldPos.x - center.x;
		const float dy = worldPos.y - center.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = body;
		}
	}
	// 内部栏点选整格，空出来的边也能把 Node 拖出来。
	if (const int inner = HitVisibleInnerSlot_(worldPos); inner >= 0)
	{
		const std::size_t index = static_cast<std::size_t>(inner);
		if (IModuleNode* held = GetInnerSlot(index))
		{
			const DirectX::XMFLOAT2 center = GetInnerSlotCenter(index);
			const float dx = worldPos.x - center.x;
			const float dy = worldPos.y - center.y;
			const float distSq = dx * dx + dy * dy;
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				best = held;
			}
		}
	}
	if (best != nullptr)
	{
		outDistSq = bestDistSq;
	}
	return best;
}

DirectX::XMFLOAT2 ModuleShopUltra::GetUltraSlotCenter() const noexcept
{
	const Layout layout = BuildLayout_();
	const float px = static_cast<float>(layout.ultraX) + static_cast<float>(layout.ultraSide) * 0.5f;
	const float py = static_cast<float>(layout.ultraY) + static_cast<float>(layout.ultraSide) * 0.5f;
	return PixelCenterToWorld_(layout, px, py);
}

float ModuleShopUltra::GetUltraSlotRadius() const noexcept
{
	const Layout layout = BuildLayout_();
	return SlotRadius_(layout, layout.ultraSide);
}

DirectX::XMFLOAT2 ModuleShopUltra::GetInnerSlotCenter(std::size_t index) const noexcept
{
	const Layout layout = BuildLayout_();
	if (layout.innerSide <= 0 || index >= static_cast<std::size_t>(layout.capacity))
	{
		return layout.bounds.center;
	}
	const int stride = layout.innerSide + layout.innerGap;
	const int delta = static_cast<int>(index) - layout.visibleBegin;
	const float px = static_cast<float>(layout.innerX0 + delta * stride) + static_cast<float>(layout.innerSide) * 0.5f;
	const float py = static_cast<float>(layout.innerY) + static_cast<float>(layout.innerSide) * 0.5f;
	return PixelCenterToWorld_(layout, px, py);
}

float ModuleShopUltra::GetInnerSlotRadius() const noexcept
{
	const Layout layout = BuildLayout_();
	return SlotRadius_(layout, layout.innerSide);
}

int ModuleShopUltra::GetInnerSlotCount() const noexcept
{
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	return (ultra != nullptr) ? ultra->GetCapacity() : 0;
}

IModuleNode* ModuleShopUltra::GetInnerSlot(std::size_t index) noexcept
{
	ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	return (ultra != nullptr) ? ultra->GetSlot(index) : nullptr;
}

const IModuleNode* ModuleShopUltra::GetInnerSlot(std::size_t index) const noexcept
{
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());
	return (ultra != nullptr) ? ultra->GetSlot(index) : nullptr;
}

bool ModuleShopUltra::IsInnerSlotVisible(std::size_t index) const noexcept
{
	const Layout layout = BuildLayout_();
	if (index >= static_cast<std::size_t>(layout.capacity))
	{
		return false;
	}
	const int slot = static_cast<int>(index);
	return slot >= layout.visibleBegin && slot < layout.visibleBegin + layout.visibleCount;
}

void ModuleShopUltra::DrawPager_(Canvas2D& canvas, const Layout& layout) noexcept
{
	const PagerGlyphs_ glyphs = BuildPagerGlyphs_(
		layout.pagerX,
		layout.pagerY,
		layout.pagerW,
		layout.pagerH,
		layout.capacity);
	if (!glyphs.valid)
	{
		return;
	}

	const bool canLeft = layout.visibleBegin > 0;
	const bool canRight = layout.visibleBegin + layout.windowCount < layout.capacity;
	DrawPagerArrow_(canvas, glyphs.leftX, glyphs.triY, glyphs.triW, glyphs.triH, false, canLeft);
	DrawPagerArrow_(canvas, glyphs.rightX, glyphs.triY, glyphs.triW, glyphs.triH, true, canRight);
	for (int i = 0; i < glyphs.count; ++i)
	{
		const bool onScreen = i >= layout.visibleBegin && i < layout.visibleBegin + layout.visibleCount;
		DrawPagerDot_(canvas, glyphs.firstCx + i * glyphs.stride, glyphs.cy, glyphs.radius, onScreen);
	}
}

void ModuleShopUltra::Paint_()
{
	if (panel_ == nullptr)
	{
		return;
	}

	Canvas2D& canvas = *panel_;
	ModuleShopDraw::PaintBandChrome_(canvas);
	const Layout layout = BuildLayout_();
	const int currency = GameStatsCodex::GetCurrency();
	const ModuleNode_Ultra* ultra = AsUltra_(ultra_.get());

	if (ultra_ == nullptr)
	{
		// 无奥义：区域正中只有锻造。钱够才亮，亮了数字变黄。
		const bool lit = currency >= GameRulesCodex::ForgeCost;
		DrawActionButton_(
			canvas,
			layout.forgeX,
			layout.forgeY,
			layout.forgeW,
			layout.forgeH,
			GetUiCopy("shop.ultra.forge"),
			GameRulesCodex::ForgeCost,
			lit);
	}
	else
	{
		DrawSlot_(canvas, layout.ultraX, layout.ultraY, layout.ultraSide);
		const int stride = layout.innerSide + layout.innerGap;
		for (int i = 0; i < layout.visibleCount; ++i)
		{
			const int x = layout.innerX0 + i * stride;
			DrawSlot_(canvas, x, layout.innerY, layout.innerSide);
		}
		DrawPager_(canvas, layout);

		const std::string captions[kButtonCount_] = {
			GetUiCopy("shop.ultra.punch"),
			GetUiCopy("shop.ultra.rename"),
			GetUiCopy("shop.ultra.redraw")
		};
		const int costs[kButtonCount_] = { GameRulesCodex::PunchCost, -1, -1 };
		const bool lits[kButtonCount_] = {
			currency >= GameRulesCodex::PunchCost,
			true,
			true
		};
		for (int i = 0; i < kButtonCount_; ++i)
		{
			const int y = layout.btnY + i * (layout.btnH + layout.btnGap);
			DrawActionButton_(canvas, layout.btnX, y, layout.btnW, layout.btnH, captions[i], costs[i], lits[i]);
		}
	}

	painted_ = true;
	paintedHas_ = ultra_ != nullptr;
	paintedScroll_ = scroll_;
	paintedCurrency_ = currency;
	paintedCapacity_ = (ultra != nullptr) ? ultra->GetCapacity() : 0;
	paintedRevision_ = (ultra != nullptr) ? ultra->GetContentRevision() : 0u;
	canvas.NotifyPixelsChanged();
}
