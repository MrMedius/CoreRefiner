#include "ModuleShopPanel.h"

#include "Canvas.h"
#include "Canvas2D.h"
#include "CanvasPixelDraw.h"
#include "CanvasTextDraw.h"
#include "Channels.h"
#include "Colors.h"
#include "Graphics.h"
#include "IModuleNode.h"
#include "ModuleNodeInfoCopy.h"
#include "ModuleNodes.h"
#include "RenderGraph.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

ModuleShopPanel::~ModuleShopPanel() = default;

namespace
{
	constexpr Color kCardBg{ 42u, 56u, 78u, 190u };
	constexpr Color kCardBgLocked{ 22u, 30u, 42u, 230u };
	constexpr Color kEmptyBg{ 28u, 36u, 52u, 150u };
	constexpr Color kFrame{ 170u, 190u, 210u, 150u };
	constexpr Color kFrameLocked{ 88u, 104u, 124u, 210u };
	constexpr Color kMoneyYellow{ 255u, 220u, 90u, 255u };
	constexpr std::string_view kKindSep{ " / " };

	[[nodiscard]] float CardFontSize_(float logical) noexcept
	{
		return logical * static_cast<float>(ModuleShopPanel::TexelScale());
	}

	[[nodiscard]] int CardLineH_()
	{
		const unsigned h = CanvasTextDraw::Measure(
			"Lv.Max",
			CardFontSize_(ModuleShopPanel::kHeaderFontSize),
			false,
			4096.0f).heightPx;
		return static_cast<int>((std::max)(1u, h));
	}

	[[nodiscard]] Color ShopKindCopyColor_(ModuleNodeKind kind) noexcept
	{
		if (kind == ModuleNodeKind::Fusion)
		{
			return Colors::White;
		}
		const std::size_t i = ToIndex(kind);
		if (i >= ModuleNodeKindCount())
		{
			return Colors::White;
		}
		const Color c = ModuleNodeKindFill::kFill[i];
		if (c.GetA() == 0u)
		{
			return Colors::White;
		}
		return c;
	}

	void CollectShopHeaderKinds_(
		const IModuleNode& node,
		std::vector<ModuleNodeKind>& out)
	{
		out.clear();
		out.push_back(node.GetKind());
		if (node.GetKind() != ModuleNodeKind::Fusion)
		{
			return;
		}

		const auto& fusion = static_cast<const ModuleNode_Fusion&>(node);
		if (const IModuleNode* primary = fusion.GetPrimary())
		{
			out.push_back(primary->GetKind());
		}
		if (const IModuleNode* material = fusion.GetMaterial())
		{
			out.push_back(material->GetKind());
		}
	}

	void BuildShopKindLine_(
		const std::vector<ModuleNodeKind>& kinds,
		std::string& text,
		std::vector<Text::Span>& spans)
	{
		text.clear();
		spans.clear();
		for (std::size_t i = 0; i < kinds.size(); ++i)
		{
			if (i > 0)
			{
				text += kKindSep;
			}
			const std::string name = GetKindCopy(kinds[i]);
			Text::Span span{};
			span.start = static_cast<UINT32>(Utf16CodeUnitCount(text));
			span.length = static_cast<UINT32>(Utf16CodeUnitCount(name));
			span.color = ShopKindCopyColor_(kinds[i]);
			spans.push_back(span);
			text += name;
		}
	}
}

int ModuleShopPanel::TexelScale() noexcept
{
	return (std::max)(1, kTexelScale);
}

int ModuleShopPanel::IconSidePx() noexcept
{
	const float n = static_cast<float>(TexelScale());
	return (std::max)(1, static_cast<int>(std::lround(2.0f * kIconWorldRadius * n)));
}

void ModuleShopPanel::Ensure(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	const unsigned n = static_cast<unsigned>(TexelScale());
	const unsigned w = (std::max)(1u, static_cast<unsigned>(std::lround(kWidth)) * n);
	const unsigned h = (std::max)(1u, static_cast<unsigned>(std::lround(kHeight)) * n);
	if (canvas_ == nullptr)
	{
		canvas_ = std::make_unique<Canvas2D>(gfx, w, h);
		canvas_->LinkTechniques(rg);
		return;
	}
	if (canvas_->GetCanvasWidth() != w || canvas_->GetCanvasHeight() != h)
	{
		canvas_->Resize(w, h);
	}
}

void ModuleShopPanel::SetWorldCenter(DirectX::XMFLOAT3 center) noexcept
{
	if (canvas_ == nullptr)
	{
		return;
	}
	canvas_->SetPosition(center);
	canvas_->SetScale(DirectX::XMFLOAT3{ kWidth, kHeight, 1.0f });
}

void ModuleShopPanel::Submit() const
{
	if (canvas_ != nullptr)
	{
		canvas_->Submit(Chan::ui);
	}
}

void ModuleShopPanel::PaintChrome_(bool empty, bool locked)
{
	Canvas2D& canvas = *canvas_;
	const Color bg = empty ? kEmptyBg : (locked ? kCardBgLocked : kCardBg);
	const Color frame = locked ? kFrameLocked : kFrame;
	canvas.Clear(bg);
	const int cw = static_cast<int>(canvas.GetCanvasWidth());
	const int ch = static_cast<int>(canvas.GetCanvasHeight());
	const int texel = TexelScale();
	CanvasPixelDraw::DrawRectOutlineThick(canvas, texel, texel, cw - 1 - texel, ch - 1 - texel, texel, frame);
}

void ModuleShopPanel::PaintSold_()
{
	PaintChrome_(true, false);
	Canvas2D& canvas = *canvas_;
	const int texel = TexelScale();
	constexpr Color kSold{ 160u, 160u, 160u, 220u };
	CanvasTextDraw::Draw(
		canvas,
		"SOLD OUT",
		CardFontSize_(16.0f),
		0,
		0,
		static_cast<int>(canvas.GetCanvasWidth()),
		static_cast<int>(canvas.GetCanvasHeight()),
		kSold,
		true,
		{},
		6 * texel,
		DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	canvas.NotifyPixelsChanged();
}

void ModuleShopPanel::PaintStock_(const IModuleNode& node, int price, bool locked)
{
	PaintChrome_(false, locked);
	Canvas2D& canvas = *canvas_;
	const Color frame = locked ? kFrameLocked : kFrame;
	const int cw = static_cast<int>(canvas.GetCanvasWidth());
	const int ch = static_cast<int>(canvas.GetCanvasHeight());
	const int texel = TexelScale();
	const int pad = static_cast<int>(kIconPad) * texel;
	const int lineH = CardLineH_();
	const int iconSide = IconSidePx();
	const int headerGap = kHeaderLineGap * texel;
	const float headerFont = CardFontSize_(kHeaderFontSize);
	const float bodyFont = CardFontSize_(kBodyFontSize);
	const float priceFont = CardFontSize_(kPriceFontSize);
	const int innerW = (std::max)(1, cw - pad * 2);

	const int titleY = pad + iconSide + pad;
	const ModuleNodeInfoEntry entry = ComposeModuleNodeInfoCopy(node);
	const int titleNatural = static_cast<int>(CanvasTextDraw::Measure(
		entry.title,
		headerFont,
		true,
		static_cast<float>(innerW)).heightPx);
	const int titleMaxH = 2 * lineH + headerGap;
	const int titleH = (std::max)(1, (std::min)(titleMaxH, (std::max)(lineH, titleNatural)));

	CanvasTextDraw::Draw(
		canvas,
		entry.title,
		headerFont,
		pad,
		titleY,
		innerW,
		titleH,
		Colors::White,
		true,
		entry.spans,
		0,
		DWRITE_TEXT_ALIGNMENT_CENTER);

	std::vector<ModuleNodeKind> kinds;
	CollectShopHeaderKinds_(node, kinds);
	std::string kindLine;
	std::vector<Text::Span> kindSpans;
	BuildShopKindLine_(kinds, kindLine, kindSpans);
	const int kindY = titleY + titleH + headerGap;
	CanvasTextDraw::Draw(
		canvas,
		kindLine,
		headerFont,
		pad,
		kindY,
		innerW,
		lineH,
		Colors::White,
		false,
		kindSpans,
		0,
		DWRITE_TEXT_ALIGNMENT_CENTER);

	const int headerBottom = kindY + lineH + pad;
	CanvasTextDraw::DrawHRule(canvas, pad, cw - pad - 1, headerBottom, frame, texel);

	const int priceH = static_cast<int>(kPriceFontSize) * texel + pad;
	const int priceY = ch - pad - priceH;
	const int priceRuleY = priceY - pad;
	if (priceRuleY > headerBottom)
	{
		CanvasTextDraw::DrawHRule(canvas, pad, cw - pad - 1, priceRuleY, frame, texel);
	}

	const std::vector<std::string> bodyBlocks = entry.BodyBlocks();
	int cursorY = headerBottom + pad;
	const int bodyLimitY = (std::max)(cursorY, priceRuleY);
	bool drewBody = false;
	for (std::size_t i = 0; i < bodyBlocks.size(); ++i)
	{
		const std::string& block = bodyBlocks[i];
		if (block.empty() || cursorY >= bodyLimitY)
		{
			continue;
		}
		if (drewBody)
		{
			const int splitY = cursorY;
			if (splitY < bodyLimitY)
			{
				CanvasTextDraw::DrawHRule(canvas, pad, cw - pad - 1, splitY, frame, texel);
			}
			cursorY = splitY + texel + pad;
		}
		const int remain = bodyLimitY - cursorY;
		if (remain <= 0)
		{
			break;
		}
		const int naturalH = static_cast<int>(CanvasTextDraw::Measure(
			block,
			bodyFont,
			true,
			static_cast<float>(innerW)).heightPx);
		const int h = (std::max)(1, (std::min)(remain, (std::max)(1, naturalH)));
		CanvasTextDraw::Draw(
			canvas,
			block,
			bodyFont,
			pad,
			cursorY,
			innerW,
			h,
			Colors::White,
			true);
		cursorY += h;
		drewBody = true;
	}

	CanvasTextDraw::Draw(
		canvas,
		std::to_string(price),
		priceFont,
		pad,
		priceY,
		innerW,
		priceH,
		kMoneyYellow,
		true,
		{},
		0,
		DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	canvas.NotifyPixelsChanged();
}

void ModuleShopPanel::Rebuild(const IModuleNode* node, int price, bool sold, bool locked)
{
	if (canvas_ == nullptr)
	{
		return;
	}
	if (sold)
	{
		PaintSold_();
		return;
	}
	if (node == nullptr)
	{
		PaintChrome_(true, false);
		canvas_->NotifyPixelsChanged();
		return;
	}
	PaintStock_(*node, price, locked);
}
