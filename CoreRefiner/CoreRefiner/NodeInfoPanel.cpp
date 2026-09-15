#include "NodeInfoPanel.h"

#include "Canvas.h"
#include "Canvas2D.h"
#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Colors.h"
#include "GameStatsCodex.h"
#include "Graphics.h"
#include "IconAtlas.h"
#include "IModuleNode.h"
#include "ModuleNodes.h"
#include "RenderGraph.h"
#include "TextCodex.h"
#include "UiCopy.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	constexpr Color kPanelBg{ 24u, 26u, 32u, 220u };
	constexpr Color kPanelFrame{ 200u, 205u, 215u, 200u };
	constexpr Color kHeaderRule{ 200u, 205u, 215u, 160u };
	// Fusion Kind 词条白底（只垫在那几个字后面）
	constexpr Color kFusionKindBg{ 255u, 255u, 255u, 255u };
	// 与商店货卡/顶栏/炼成费用同色
	constexpr Color kMoneyYellow{ 255u, 220u, 90u, 255u };
	constexpr std::string_view kKindSep{ " / " };

	[[nodiscard]] DirectX::XMFLOAT2 ClampPanelCenter_(
		float cx,
		float cy,
		float halfW,
		float halfH,
		float pad) noexcept
	{
		const float minX = pad + halfW;
		const float maxX = static_cast<float>(SCREEN_WIDTH) - pad - halfW;
		const float minY = pad + halfH;
		const float maxY = static_cast<float>(SCREEN_HEIGHT) - pad - halfH;

		if (minX <= maxX)
		{
			cx = std::clamp(cx, minX, maxX);
		}
		else
		{
			cx = static_cast<float>(SCREEN_WIDTH) * 0.5f;
		}

		if (minY <= maxY)
		{
			cy = std::clamp(cy, minY, maxY);
		}
		else
		{
			cy = static_cast<float>(SCREEN_HEIGHT) * 0.5f;
		}

		return DirectX::XMFLOAT2{ cx, cy };
	}

	void FillPanelText_(Text::RenderRequest& rq, const std::string& text, float fontSize, Color color)
	{
		rq.text = text;
		rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		rq.fallbackFonts.clear();
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Yu Gothic UI"));
		rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
		rq.style.fontSize = fontSize;
		rq.defaultColor = color;
	}

	[[nodiscard]] Color KindCopyColor_(ModuleNodeKind kind) noexcept
	{
		if (kind == ModuleNodeKind::Fusion)
		{
			return Color(0u, 0u, 0u, 255u);
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

	[[nodiscard]] Color KindIconColor_(ModuleNodeKind kind) noexcept
	{
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

	void CollectHeaderKinds_(
		const IModuleNode* node,
		ModuleNodeLabel label,
		std::vector<ModuleNodeKind>& out)
	{
		out.clear();
		if (node == nullptr)
		{
			const ModuleNodeKind kind = KindOf(label);
			if (kind < ModuleNodeKind::Count)
			{
				out.push_back(kind);
			}
			return;
		}

		out.push_back(node->GetKind());
		if (node->GetKind() != ModuleNodeKind::Fusion)
		{
			return;
		}

		const auto& fusion = static_cast<const ModuleNode_Fusion&>(*node);
		if (const IModuleNode* primary = fusion.GetPrimary())
		{
			out.push_back(primary->GetKind());
		}
		if (const IModuleNode* material = fusion.GetMaterial())
		{
			out.push_back(material->GetKind());
		}
	}

	void BuildKindLine_(
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
			span.color = KindCopyColor_(kinds[i]);
			spans.push_back(span);
			text += name;
		}
	}

	[[nodiscard]] Text::MeasureResult MeasurePanelText_(
		const std::string& text,
		float fontSize,
		const std::vector<Text::Span>& spans)
	{
		Text::MeasureResult result{};
		result.widthPx = 0u;
		result.heightPx = 0u;
		if (text.empty())
		{
			return result;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillPanelText_(rq, text, fontSize, Colors::White);
		rq.canvasMode = Text::CanvasMode::Auto;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.paddingPx = 0;
		rq.style.wordWrapEnabled = false;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		rq.maxWidthPx = 4096.0f;
		rq.spans = spans;
		return ctx.Measure();
	}

	[[nodiscard]] Text::MeasureResult MeasurePanelBody_(
		const std::string& text,
		float fontSize,
		float maxWidthPx)
	{
		Text::MeasureResult result{};
		result.widthPx = 0u;
		result.heightPx = 0u;
		if (text.empty() || maxWidthPx <= 0.0f)
		{
			return result;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillPanelText_(rq, text, fontSize, Colors::White);
		rq.canvasMode = Text::CanvasMode::Auto;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.paddingPx = 0;
		rq.style.wordWrapEnabled = true;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		rq.maxWidthPx = maxWidthPx;
		return ctx.Measure();
	}

	[[nodiscard]] std::string FormatStatNumber_(float value)
	{
		char buf[32]{};
		const float rounded = std::round(value);
		if (std::fabs(value - rounded) < 0.05f)
		{
			std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(rounded));
		}
		else
		{
			std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(value));
		}
		return buf;
	}

	void DrawPanelText_(
		Canvas2D& canvas,
		const std::string& text,
		float fontSize,
		int boxX,
		int boxY,
		int boxW,
		int boxH,
		Color color,
		const std::vector<Text::Span>& spans,
		bool wrap,
		DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING)
	{
		if (text.empty() || boxW <= 0 || boxH <= 0)
		{
			return;
		}

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		FillPanelText_(rq, text, fontSize, color);
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.style.wordWrapEnabled = wrap;
		rq.style.textAlign = align;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		rq.paddingPx = 0;
		rq.SetDestRect(
			static_cast<float>(boxX),
			static_cast<float>(boxY),
			static_cast<float>(boxW),
			static_cast<float>(boxH));
		rq.backgroundColor = Colors::None;
		rq.spans = spans;
		ctx.Render(canvas);
	}

	void BlitHeaderIcon_(
		Canvas& dst,
		const IModuleNode* node,
		ModuleNodeLabel label,
		int dx,
		int dy,
		unsigned side)
	{
		Canvas src{
			IModuleNode::kVisualSize,
			IModuleNode::kVisualSize,
			Canvas::Empty
		};
		src.Clear(Colors::None);
		if (node != nullptr)
		{
			node->BlitIcon(src, false);
		}
		else
		{
			const ModuleNodeKind kind = KindOf(label);
			IconAtlas::BlitIcon(
				src,
				NodeIconAtlas::Get(label),
				KindIconColor_(kind));
		}
		CanvasPixelDraw::BlitNearestCentered(dst, src, dx, dy, side, side);
	}

	struct PanelStatRow
	{
		std::string label;
		std::string value;
	};

	[[nodiscard]] int StatBlockHeight_(std::size_t n, int lineH, int lineGap)
	{
		if (n == 0u || lineH <= 0)
		{
			return 0;
		}
		return static_cast<int>(n) * lineH + static_cast<int>(n - 1u) * lineGap;
	}

	[[nodiscard]] unsigned MaxValueColWidth_(const std::vector<PanelStatRow>& rows, float fontSize)
	{
		unsigned w = 1u;
		for (const PanelStatRow& row : rows)
		{
			w = (std::max)(w, MeasurePanelText_(row.value, fontSize, {}).widthPx);
		}
		return w;
	}

	void FillFusionKindBacks_(
		Canvas& dst,
		const std::vector<ModuleNodeKind>& kinds,
		float fontSize,
		int textX,
		int rowY,
		int lineH,
		int maxX)
	{
		if (lineH <= 0 || textX >= maxX || rowY < 0)
		{
			return;
		}

		std::string prefix;
		for (std::size_t i = 0; i < kinds.size(); ++i)
		{
			if (i > 0)
			{
				prefix += kKindSep;
			}
			const std::string name = GetKindCopy(kinds[i]);
			if (kinds[i] == ModuleNodeKind::Fusion && !name.empty())
			{
				const int x0 = textX + static_cast<int>(MeasurePanelText_(prefix, fontSize, {}).widthPx);
				const int x1 = textX + static_cast<int>(MeasurePanelText_(prefix + name, fontSize, {}).widthPx) - 1;
				int left = x0;
				int right = x1;
				if (left < textX)
				{
					left = textX;
				}
				if (right >= maxX)
				{
					right = maxX - 1;
				}
				if (right >= left)
				{
					CanvasPixelDraw::FillRect(
						dst,
						static_cast<unsigned>(left),
						static_cast<unsigned>(rowY),
						static_cast<unsigned>(right),
						static_cast<unsigned>(rowY + lineH - 1),
						kFusionKindBg);
				}
			}
			prefix += name;
		}
	}

	void AppendUniqueStatRows_(const IModuleNode* src, std::vector<PanelStatRow>& dst)
	{
		if (src == nullptr)
		{
			return;
		}
		std::vector<ModuleNodeUniqueStatRow> raw;
		src->CollectUniqueStats(raw);
		for (const ModuleNodeUniqueStatRow& row : raw)
		{
			ModuleNodeStat stat = ModuleNodeStat::Count;
			switch (row.id)
			{
			case ModuleNodeUniqueStat::LifetimeRate:
				stat = ModuleNodeStat::LifetimeRate;
				break;
			case ModuleNodeUniqueStat::SpeedRate:
				stat = ModuleNodeStat::SpeedRate;
				break;
			case ModuleNodeUniqueStat::SizeRate:
				stat = ModuleNodeStat::SizeRate;
				break;
			case ModuleNodeUniqueStat::DamageRate:
				stat = ModuleNodeStat::DamageRate;
				break;
			case ModuleNodeUniqueStat::DamageFix:
				stat = ModuleNodeStat::DamageFix;
				break;
			case ModuleNodeUniqueStat::RepeatCount:
				stat = ModuleNodeStat::RepeatCount;
				break;
			default:
				break;
			}
			if (stat == ModuleNodeStat::Count)
			{
				continue;
			}
			dst.push_back({ GetStatCopy(stat), FormatStatNumber_(row.value) });
		}
	}

	int DrawStatRows_(
		Canvas2D& canvas,
		const std::vector<PanelStatRow>& rows,
		float fontSize,
		int pad,
		int innerW,
		int valueColW,
		int y,
		int lineH,
		int lineGap,
		Color ink)
	{
		const int labelW = (std::max)(1, innerW - valueColW - 8);
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			const PanelStatRow& row = rows[i];
			DrawPanelText_(
				canvas,
				row.label,
				fontSize,
				pad,
				y,
				labelW,
				lineH,
				ink,
				{},
				false);
			DrawPanelText_(
				canvas,
				row.value,
				fontSize,
				pad + innerW - valueColW,
				y,
				valueColW,
				lineH,
				ink,
				{},
				false,
				DWRITE_TEXT_ALIGNMENT_TRAILING);
			y += lineH;
			if (i + 1u < rows.size())
			{
				y += lineGap;
			}
		}
		return y;
	}
}

void NodeInfoPanel::Ensure(Graphics& gfx, Rgph::RenderGraph& rg)
{
	gfx_ = &gfx;
	rg_ = &rg;
	if (canvas_ != nullptr)
	{
		return;
	}

	constexpr unsigned kBoot = 64u;
	canvas_ = std::make_unique<Canvas2D>(gfx, kBoot, kBoot);
	canvas_->Clear(Colors::None);
	canvas_->LinkTechniques(rg);
}

void NodeInfoPanel::ShowFor(ModuleNodeLabel label, DirectX::XMFLOAT2 anchorGameXY, Anchor anchor, float maxWidthPx)
{
	if (canvas_ == nullptr)
	{
		return;
	}

	if (maxWidthPx <= 0.0f)
	{
		maxWidthPx = kMaxWidthPx_;
	}

	const Language lang = GameStatsCodex::GetLanguage();
	const bool contentDirty =
		cachedInstanceId_.has_value()
		|| !cachedLabel_.has_value()
		|| !cachedLanguage_.has_value()
		|| *cachedLabel_ != label
		|| *cachedLanguage_ != lang
		|| maxWidthPx_ != maxWidthPx;

	anchor_ = anchor;
	maxWidthPx_ = maxWidthPx;

	if (contentDirty)
	{
		RebuildContent_(GetModuleNodeInfoCopy(label), nullptr, label);
		cachedLabel_ = label;
		cachedInstanceId_.reset();
		cachedLevel_.reset();
		cachedBuyPrice_.reset();
		cachedLanguage_ = lang;
	}

	visible_ = true;
	SyncPosition_(anchorGameXY);
}

void NodeInfoPanel::ShowFor(const IModuleNode& node, DirectX::XMFLOAT2 anchorGameXY, Anchor anchor, float maxWidthPx)
{
	if (canvas_ == nullptr)
	{
		return;
	}

	if (maxWidthPx <= 0.0f)
	{
		maxWidthPx = kMaxWidthPx_;
	}

	const Language lang = GameStatsCodex::GetLanguage();
	const bool contentDirty =
		!cachedInstanceId_.has_value()
		|| !cachedLanguage_.has_value()
		|| *cachedInstanceId_ != node.GetInstanceId()
		|| *cachedLanguage_ != lang
		|| maxWidthPx_ != maxWidthPx
		|| !cachedLevel_.has_value()
		|| *cachedLevel_ != node.GetLevel()
		|| !cachedBuyPrice_.has_value()
		|| *cachedBuyPrice_ != node.GetBuyPrice();

	anchor_ = anchor;
	maxWidthPx_ = maxWidthPx;

	if (contentDirty)
	{
		RebuildContent_(ComposeModuleNodeInfoCopy(node), &node, node.GetModuleNodeLabel());
		cachedLabel_ = node.GetModuleNodeLabel();
		cachedInstanceId_ = node.GetInstanceId();
		cachedLevel_ = node.GetLevel();
		cachedBuyPrice_ = node.GetBuyPrice();
		cachedLanguage_ = lang;
	}

	visible_ = true;
	SyncPosition_(anchorGameXY);
}

void NodeInfoPanel::Hide() noexcept
{
	visible_ = false;
}

void NodeInfoPanel::Submit() const
{
	if (!visible_ || canvas_ == nullptr)
	{
		return;
	}
	canvas_->Submit(Chan::ui);
}

void NodeInfoPanel::RebuildContent_(
	const ModuleNodeInfoEntry& entry,
	const IModuleNode* node,
	ModuleNodeLabel label)
{
	std::vector<ModuleNodeKind> kinds;
	CollectHeaderKinds_(node, label, kinds);

	std::string kindLine;
	std::vector<Text::Span> kindSpans;
	BuildKindLine_(kinds, kindLine, kindSpans);

	std::string levelLine;
	if (node != nullptr)
	{
		if (node->GetLevel() >= ModuleNodeLevel::kMax)
		{
			levelLine = "Lv.Max";
		}
		else
		{
			levelLine = "Lv." + std::to_string(node->GetLevel());
		}
	}

	std::string priceLine;
	std::vector<Text::Span> priceSpans;
	if (node != nullptr)
	{
		const std::string costLabel = GetUiCopy("node.cost");
		const std::string cost = std::to_string(node->GetBuyPrice());
		priceLine = costLabel + " " + cost;
		Text::Span money{};
		money.start = static_cast<UINT32>(
			Utf16CodeUnitCount(costLabel) + Utf16CodeUnitCount(" "));
		money.length = static_cast<UINT32>(Utf16CodeUnitCount(cost));
		money.color = kMoneyYellow;
		priceSpans.push_back(money);
	}

	const Text::MeasureResult lineProbe = MeasurePanelText_("Lv.Max", kFontSize_, {});
	const int lineH = static_cast<int>((std::max)(1u, lineProbe.heightPx));
	const int iconSide = 4 * lineH + 3 * kHeaderLineGap_;

	std::vector<PanelStatRow> baseStats;
	if (node != nullptr)
	{
		baseStats.push_back({ GetStatCopy(ModuleNodeStat::NodeRadius), FormatStatNumber_(node->GetHitRadius()) });
		baseStats.push_back({ GetStatCopy(ModuleNodeStat::Cooldown), FormatStatNumber_(node->GetCooldownDuration()) });
		baseStats.push_back({ GetStatCopy(ModuleNodeStat::ExpandRadius), FormatStatNumber_(node->GetScanMaxRadius()) });
		baseStats.push_back({ GetStatCopy(ModuleNodeStat::ExpandSpeed), FormatStatNumber_(node->GetScanExpandSpeed()) });
	}

	std::vector<PanelStatRow> uniquePrimary;
	std::vector<PanelStatRow> uniqueMaterial;
	if (node != nullptr)
	{
		if (node->GetKind() == ModuleNodeKind::Fusion)
		{
			const auto& fusion = static_cast<const ModuleNode_Fusion&>(*node);
			AppendUniqueStatRows_(fusion.GetPrimary(), uniquePrimary);
			AppendUniqueStatRows_(fusion.GetMaterial(), uniqueMaterial);
		}
		else
		{
			AppendUniqueStatRows_(node, uniquePrimary);
		}
	}
	const bool hasUnique = !uniquePrimary.empty() || !uniqueMaterial.empty();
	const bool uniqueSplit = !uniquePrimary.empty() && !uniqueMaterial.empty();

	const int pad = kPaddingPx_;
	const int panelW = (std::max)(pad * 2 + 1, static_cast<int>(maxWidthPx_));
	const int innerW = (std::max)(1, panelW - pad * 2);
	const std::vector<std::string> bodyBlocks = entry.BodyBlocks();
	std::vector<int> bodyHs;
	bodyHs.reserve(bodyBlocks.size());
	int bodySectionH = 0;
	int bodyDrawnCount = 0;
	for (const std::string& block : bodyBlocks)
	{
		const int h = static_cast<int>(MeasurePanelBody_(
			block,
			kFontSize_,
			static_cast<float>(innerW)).heightPx);
		bodyHs.push_back(h);
		if (h <= 0)
		{
			continue;
		}
		if (bodyDrawnCount > 0)
		{
			bodySectionH += kHeaderBodyGap_ + 1 + kHeaderBodyGap_;
		}
		bodySectionH += h;
		++bodyDrawnCount;
	}

	int panelH = pad + iconSide + kHeaderRuleGap_ + 1 + pad;
	if (bodySectionH > 0)
	{
		panelH += kHeaderBodyGap_ + bodySectionH + kHeaderRuleGap_ + 1;
	}
	if (!baseStats.empty())
	{
		panelH += kHeaderBodyGap_ + StatBlockHeight_(baseStats.size(), lineH, kHeaderLineGap_);
	}
	if (hasUnique)
	{
		panelH += kHeaderBodyGap_ + 1 + kHeaderBodyGap_;
		panelH += StatBlockHeight_(uniquePrimary.size(), lineH, kHeaderLineGap_);
		panelH += StatBlockHeight_(uniqueMaterial.size(), lineH, kHeaderLineGap_);
		if (uniqueSplit)
		{
			panelH += kHeaderBodyGap_ + 1 + kHeaderBodyGap_;
		}
	}

	canvas_->Resize(static_cast<unsigned>(panelW), static_cast<unsigned>(panelH));
	canvas_->Clear(kPanelBg);

	const int iconX = pad;
	const int iconY = pad;
	const int textX = pad + iconSide + kHeaderIconTextGap_;
	BlitHeaderIcon_(*canvas_, node, label, iconX, iconY, static_cast<unsigned>(iconSide));

	const int row0Y = iconY;
	const int row1Y = row0Y + lineH + kHeaderLineGap_;
	const int row2Y = row1Y + lineH + kHeaderLineGap_;
	const int row3Y = row2Y + lineH + kHeaderLineGap_;
	const int drawTextW = (std::max)(1, panelW - textX - pad);

	DrawPanelText_(
		*canvas_,
		entry.title,
		kFontSize_,
		textX,
		row0Y,
		drawTextW,
		lineH,
		Colors::White,
		entry.spans,
		false);
	FillFusionKindBacks_(
		*canvas_,
		kinds,
		kFontSize_,
		textX,
		row1Y,
		lineH,
		textX + drawTextW);
	DrawPanelText_(
		*canvas_,
		kindLine,
		kFontSize_,
		textX,
		row1Y,
		drawTextW,
		lineH,
		Colors::White,
		kindSpans,
		false);
	DrawPanelText_(
		*canvas_,
		levelLine,
		kFontSize_,
		textX,
		row2Y,
		drawTextW,
		lineH,
		Colors::White,
		{},
		false);
	DrawPanelText_(
		*canvas_,
		priceLine,
		kFontSize_,
		textX,
		row3Y,
		drawTextW,
		lineH,
		Colors::White,
		priceSpans,
		false);

	const int ruleY = pad + iconSide + kHeaderRuleGap_;
	CanvasPixelDraw::DrawHLine(*canvas_, pad, panelW - pad - 1, ruleY, kHeaderRule);

	int cursorY = ruleY + 1;
	if (bodySectionH > 0)
	{
		bool drewBody = false;
		for (std::size_t i = 0; i < bodyBlocks.size(); ++i)
		{
			const int h = bodyHs[i];
			if (h <= 0)
			{
				continue;
			}
			if (drewBody)
			{
				const int splitY = cursorY + kHeaderBodyGap_;
				CanvasPixelDraw::DrawHLine(*canvas_, pad, panelW - pad - 1, splitY, kHeaderRule);
				cursorY = splitY + 1;
			}
			const int bodyY = cursorY + kHeaderBodyGap_;
			DrawPanelText_(
				*canvas_,
				bodyBlocks[i],
				kFontSize_,
				pad,
				bodyY,
				innerW,
				h,
				Colors::White,
				{},
				true);
			cursorY = bodyY + h;
			drewBody = true;
		}
		const int bodyRuleY = cursorY + kHeaderRuleGap_;
		CanvasPixelDraw::DrawHLine(*canvas_, pad, panelW - pad - 1, bodyRuleY, kHeaderRule);
		cursorY = bodyRuleY + 1;
	}

	unsigned valueColU = 1u;
	valueColU = (std::max)(valueColU, MaxValueColWidth_(baseStats, kFontSize_));
	valueColU = (std::max)(valueColU, MaxValueColWidth_(uniquePrimary, kFontSize_));
	valueColU = (std::max)(valueColU, MaxValueColWidth_(uniqueMaterial, kFontSize_));
	const int valueColW = (std::min)(innerW / 2, static_cast<int>(valueColU) + 4);

	if (!baseStats.empty())
	{
		cursorY = DrawStatRows_(
			*canvas_,
			baseStats,
			kFontSize_,
			pad,
			innerW,
			valueColW,
			cursorY + kHeaderBodyGap_,
			lineH,
			kHeaderLineGap_,
			Colors::White);
	}

	if (hasUnique)
	{
		const int uniqueRuleY = cursorY + kHeaderBodyGap_;
		CanvasPixelDraw::DrawHLine(*canvas_, pad, panelW - pad - 1, uniqueRuleY, kHeaderRule);
		cursorY = uniqueRuleY + 1 + kHeaderBodyGap_;
		if (!uniquePrimary.empty())
		{
			cursorY = DrawStatRows_(
				*canvas_,
				uniquePrimary,
				kFontSize_,
				pad,
				innerW,
				valueColW,
				cursorY,
				lineH,
				kHeaderLineGap_,
				Colors::White);
		}
		if (uniqueSplit)
		{
			const int splitRuleY = cursorY + kHeaderBodyGap_;
			CanvasPixelDraw::DrawHLine(*canvas_, pad, panelW - pad - 1, splitRuleY, kHeaderRule);
			cursorY = splitRuleY + 1 + kHeaderBodyGap_;
		}
		if (!uniqueMaterial.empty())
		{
			cursorY = DrawStatRows_(
				*canvas_,
				uniqueMaterial,
				kFontSize_,
				pad,
				innerW,
				valueColW,
				cursorY,
				lineH,
				kHeaderLineGap_,
				Colors::White);
		}
	}

	contentW_ = (std::max)(1u, canvas_->GetCanvasWidth());
	contentH_ = (std::max)(1u, canvas_->GetCanvasHeight());

	CanvasPixelDraw::DrawRectOutline(
		*canvas_,
		0,
		0,
		static_cast<int>(contentW_) - 1,
		static_cast<int>(contentH_) - 1,
		kPanelFrame);

	canvas_->NotifyPixelsChanged();
	canvas_->SetScale(DirectX::XMFLOAT3{
		static_cast<float>(contentW_),
		static_cast<float>(contentH_),
		1.0f
	});
}

void NodeInfoPanel::SyncPosition_(DirectX::XMFLOAT2 anchorGameXY)
{
	if (canvas_ == nullptr)
	{
		return;
	}

	const float halfW = static_cast<float>(contentW_) * 0.5f;
	const float halfH = static_cast<float>(contentH_) * 0.5f;

	const float preferCx = anchorGameXY.x;
	const float preferCy = (anchor_ == Anchor::Below)
		? (anchorGameXY.y + halfH + kAnchorGap_)
		: (anchorGameXY.y - halfH - kAnchorGap_);
	const DirectX::XMFLOAT2 center = ClampPanelCenter_(
		preferCx,
		preferCy,
		halfW,
		halfH,
		kScreenPad_);

	canvas_->SetPosition(DirectX::XMFLOAT3{ center.x, center.y, 0.0f });
}
