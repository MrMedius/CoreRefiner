#include "NodeInfoPanel.h"

#include "Canvas.h"
#include "Canvas2D.h"
#include "CanvasPixelDraw.h"
#include "CanvasTextDraw.h"
#include "Channels.h"
#include "Colors.h"
#include "GameStatsCodex.h"
#include "Graphics.h"
#include "IconAtlas.h"
#include "IModuleNode.h"
#include "ModuleNodes.h"
#include "RenderGraph.h"
#include "TimeCodex.h"
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
	// ————————————————————————————————————————————————————
	// 基础：配色、测量、顶区图标、属性行
	// ————————————————————————————————————————————————————
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
			w = (std::max)(w, CanvasTextDraw::Measure(row.value, fontSize, false, 4096.0f).widthPx);
		}
		return w;
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
			CanvasTextDraw::Draw(
				canvas,
				row.label,
				fontSize,
				pad,
				y,
				labelW,
				lineH,
				ink,
				false);
			CanvasTextDraw::Draw(
				canvas,
				row.value,
				fontSize,
				pad + innerW - valueColW,
				y,
				valueColW,
				lineH,
				ink,
				false,
				{},
				0,
				DWRITE_TEXT_ALIGNMENT_TRAILING);
			y += lineH;
			if (i + 1u < rows.size())
			{
				y += lineGap;
			}
		}
		return y;
	}

	// ————————————————————————————————————————————————————
	// Fusion：Kind 行带上主体和素材，Fusion 词条垫白底
	// ————————————————————————————————————————————————————
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

	void FillFusionKindBacks_(
		Canvas& dst,
		const std::vector<ModuleNodeKind>& kinds,
		float fontSize,
		int textX,
		int rowY,
		int lineH,
		int maxX,
		int offsetX = 0)
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
				const int x0 = textX + offsetX + static_cast<int>(CanvasTextDraw::Measure(prefix, fontSize, false, 4096.0f).widthPx);
				const int x1 = textX + offsetX + static_cast<int>(CanvasTextDraw::Measure(prefix + name, fontSize, false, 4096.0f).widthPx) - 1;
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

	// ————————————————————————————————————————————————————
	// 普通 Node：独有属性。Fusion 对主体和素材各调一次。
	// ————————————————————————————————————————————————————
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

	// ————————————————————————————————————————————————————
	// Ultra：按面板像素染斜向彩虹。背景不动，字滚动时穿过这层颜色。
	// ————————————————————————————————————————————————————
	void TintDiagonalRainbow_(Canvas& canvas, int x, int y, int w, int h, float phase)
	{
		if (w <= 0 || h <= 0)
		{
			return;
		}
		const int canvasW = static_cast<int>(canvas.GetCanvasWidth());
		const int canvasH = static_cast<int>(canvas.GetCanvasHeight());
		const int x0 = (std::max)(x, 0);
		const int y0 = (std::max)(y, 0);
		const int x1 = (std::min)(x + w, canvasW);
		const int y1 = (std::min)(y + h, canvasH);
		for (int py = y0; py < y1; ++py)
		{
			for (int px = x0; px < x1; ++px)
			{
				const Color cur = canvas.GetPixel(static_cast<unsigned>(px), static_cast<unsigned>(py));
				if (cur.GetR() == kPanelBg.GetR()
					&& cur.GetG() == kPanelBg.GetG()
					&& cur.GetB() == kPanelBg.GetB()
					&& cur.GetA() == kPanelBg.GetA())
				{
					continue;
				}
				const float hue = (static_cast<float>(px + py) + phase) / IconAtlas::kRainbowPeriod;
				canvas.PutPixel(static_cast<unsigned>(px), static_cast<unsigned>(py), IconAtlas::HueToRgb(hue));
			}
		}
	}
}

// ————————————————————————————————————————————————————
// 基础：显示、缓存、定位、跑马灯
// ————————————————————————————————————————————————————
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
		cachedUltraRevision_.reset();
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
	const auto* ultra = dynamic_cast<const ModuleNode_Ultra*>(&node);
	const bool ultraDirty = ultra != nullptr
		&& (!cachedUltraRevision_.has_value() || *cachedUltraRevision_ != ultra->GetContentRevision());
	const bool contentDirty =
		!cachedInstanceId_.has_value()
		|| !cachedLanguage_.has_value()
		|| *cachedInstanceId_ != node.GetInstanceId()
		|| *cachedLanguage_ != lang
		|| maxWidthPx_ != maxWidthPx
		|| !cachedLevel_.has_value()
		|| *cachedLevel_ != node.GetLevel()
		|| !cachedBuyPrice_.has_value()
		|| *cachedBuyPrice_ != node.GetBuyPrice()
		|| ultraDirty;

	anchor_ = anchor;
	maxWidthPx_ = maxWidthPx;

	if (contentDirty)
	{
		RebuildContent_(ComposeModuleNodeInfoCopy(node), &node, node.GetModuleNodeLabel());
		cachedLabel_ = node.GetModuleNodeLabel();
		cachedInstanceId_ = node.GetInstanceId();
		cachedLevel_ = node.GetLevel();
		cachedBuyPrice_ = node.GetBuyPrice();
		if (ultra != nullptr)
		{
			cachedUltraRevision_ = ultra->GetContentRevision();
		}
		else
		{
			cachedUltraRevision_.reset();
		}
		cachedLanguage_ = lang;
	}

	visible_ = true;
	SyncPosition_(anchorGameXY);
}

void NodeInfoPanel::Hide() noexcept
{
	visible_ = false;
}

void NodeInfoPanel::Submit()
{
	if (!visible_ || canvas_ == nullptr)
	{
		return;
	}
	TickMarquee_();
	TickHeaderRainbow_();
	canvas_->Submit(Chan::ui);
}

// ————————————————————————————————————————————————————
// 绘制：基础顶区；普通 Node 和 Fusion 画正文与独有属性；Ultra 改画栏位网格
// ————————————————————————————————————————————————————
void NodeInfoPanel::RebuildContent_(
	const ModuleNodeInfoEntry& entry,
	const IModuleNode* node,
	ModuleNodeLabel label)
{
	marquees_.clear();
	headerRainbowBits_.reset();
	headerRainbowBoxes_.clear();

	std::vector<ModuleNodeKind> kinds;
	CollectHeaderKinds_(node, label, kinds);

	std::string kindLine;
	std::vector<Text::Span> kindSpans;
	BuildKindLine_(kinds, kindLine, kindSpans);

	const ModuleNode_Ultra* ultra = (node != nullptr)
		? dynamic_cast<const ModuleNode_Ultra*>(node)
		: nullptr;

	std::string levelLine;
	if (node != nullptr)
	{
		if (ultra != nullptr)
		{
			// 奥义显示容量，不走等级，也不显示 Max。
			levelLine = "Lv." + std::to_string(ultra->GetCapacity());
		}
		else if (node->GetLevel() >= ModuleNodeLevel::kMax)
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

	const Text::MeasureResult lineProbe = CanvasTextDraw::Measure("Lv.Max", kFontSize_, false, 4096.0f);
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
	// Fusion 分主体和素材。普通 Node 只列自己。奥义不列独有属性。
	if (node != nullptr && ultra == nullptr)
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

	// 普通 Node 和 Fusion 量正文。Ultra 跳过正文，改量栏位网格。
	const int pad = kPaddingPx_;
	const int panelW = (std::max)(pad * 2 + 1, static_cast<int>(maxWidthPx_));
	const int innerW = (std::max)(1, panelW - pad * 2);
	// 奥义中段不画描述。
	const std::vector<std::string> bodyBlocks = (ultra != nullptr)
		? std::vector<std::string>{}
		: entry.BodyBlocks();
	std::vector<int> bodyHs;
	bodyHs.reserve(bodyBlocks.size());
	int bodySectionH = 0;
	int bodyDrawnCount = 0;
	for (const std::string& block : bodyBlocks)
	{
		const int h = static_cast<int>(CanvasTextDraw::Measure(
			block,
			kFontSize_,
			true,
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

	// 空栏位跳过。一行 5 个：5*边长 + 4*间距 + 2*边距 ≈ 面板宽，第 6 个才换行。
	std::vector<const IModuleNode*> slotIcons;
	constexpr int kUltraGridCols = 5;
	int gridCols = 1;
	int gridH = 0;
	const int slotBudget = panelW - (kUltraGridCols - 1) * kHeaderLineGap_ - 2 * pad;
	const int slotSide = (std::max)(1, (slotBudget + kUltraGridCols / 2) / kUltraGridCols);
	if (ultra != nullptr)
	{
		gridCols = kUltraGridCols;
		for (int i = 0; i < ultra->GetCapacity(); ++i)
		{
			if (const IModuleNode* slot = ultra->GetSlot(static_cast<std::size_t>(i)))
			{
				slotIcons.push_back(slot);
			}
		}
		if (!slotIcons.empty())
		{
			const int rows = static_cast<int>(
				(slotIcons.size() + static_cast<std::size_t>(gridCols) - 1u) / static_cast<std::size_t>(gridCols));
			gridH = rows * slotSide + (rows - 1) * kHeaderLineGap_;
		}
	}

	int panelH = pad + iconSide + kHeaderRuleGap_ + 1 + pad;
	if (bodySectionH > 0)
	{
		panelH += kHeaderBodyGap_ + bodySectionH + kHeaderRuleGap_ + 1;
	}
	if (gridH > 0)
	{
		panelH += kHeaderBodyGap_ + gridH + kHeaderRuleGap_ + 1;
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
	if (ultra != nullptr)
	{
		headerRainbowBits_ = ultra->GetIconBits();
		headerIconX_ = iconX;
		headerIconY_ = iconY;
		headerIconSide_ = iconSide;
	}
	BlitHeaderIcon_(*canvas_, node, label, iconX, iconY, static_cast<unsigned>(iconSide));

	const int row0Y = iconY;
	const int row1Y = row0Y + lineH + kHeaderLineGap_;
	const int row2Y = row1Y + lineH + kHeaderLineGap_;
	const int row3Y = row2Y + lineH + kHeaderLineGap_;
	const int drawTextW = (std::max)(1, panelW - textX - pad);

	auto considerHeaderMarquee = [&](
		std::string text,
		std::vector<Text::Span> spans,
		int rowY,
		bool fusionBack,
		const std::vector<ModuleNodeKind>& fusionKinds)
	{
		if (text.empty())
		{
			return;
		}
		const float textW = static_cast<float>(CanvasTextDraw::Measure(text, kFontSize_, false, 4096.0f, spans).widthPx);
		if (textW <= static_cast<float>(drawTextW) + 0.5f)
		{
			return;
		}
		HeaderMarqueeLine_ row{};
		row.text = std::move(text);
		row.spans = std::move(spans);
		if (fusionBack)
		{
			row.fusionKinds = fusionKinds;
		}
		row.color = Colors::White;
		row.boxX = textX;
		row.boxY = rowY;
		row.boxW = drawTextW;
		row.boxH = lineH;
		row.textW = textW;
		row.holdRemain = kMarqueeHoldSec_;
		row.paintFusionKindBack = fusionBack;
		marquees_.push_back(std::move(row));
	};

	// 奥义这两行先画白字，提交时再按像素染彩虹。其它 Node 仍用原来的纯色。
	const std::vector<Text::Span> titleSpans = (ultra != nullptr) ? std::vector<Text::Span>{} : entry.spans;
	const std::vector<Text::Span> kindDrawSpans = (ultra != nullptr) ? std::vector<Text::Span>{} : kindSpans;

	// 标题行高度固定为 lineH。名字为空时 Draw 直接返回，这一行仍然占位。
	CanvasTextDraw::Draw(
		*canvas_,
		entry.title,
		kFontSize_,
		textX,
		row0Y,
		drawTextW,
		lineH,
		Colors::White,
		false,
		titleSpans);
	considerHeaderMarquee(entry.title, titleSpans, row0Y, false, {});
	FillFusionKindBacks_(
		*canvas_,
		kinds,
		kFontSize_,
		textX,
		row1Y,
		lineH,
		textX + drawTextW);
	CanvasTextDraw::Draw(
		*canvas_,
		kindLine,
		kFontSize_,
		textX,
		row1Y,
		drawTextW,
		lineH,
		Colors::White,
		false,
		kindDrawSpans);
	bool kindNeedsBack = false;
	for (ModuleNodeKind k : kinds)
	{
		if (k == ModuleNodeKind::Fusion)
		{
			kindNeedsBack = true;
			break;
		}
	}
	considerHeaderMarquee(kindLine, kindDrawSpans, row1Y, kindNeedsBack, kinds);
	if (ultra != nullptr)
	{
		headerRainbowBoxes_.push_back({ textX, row0Y, drawTextW, lineH });
		headerRainbowBoxes_.push_back({ textX, row1Y, drawTextW, lineH });
	}
	CanvasTextDraw::Draw(
		*canvas_,
		levelLine,
		kFontSize_,
		textX,
		row2Y,
		drawTextW,
		lineH,
		Colors::White,
		false);
	CanvasTextDraw::Draw(
		*canvas_,
		priceLine,
		kFontSize_,
		textX,
		row3Y,
		drawTextW,
		lineH,
		Colors::White,
		false,
		priceSpans);
	considerHeaderMarquee(priceLine, priceSpans, row3Y, false, {});

	const int ruleY = pad + iconSide + kHeaderRuleGap_;
	CanvasTextDraw::DrawHRule(*canvas_, pad, panelW - pad - 1, ruleY, kHeaderRule);

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
				CanvasTextDraw::DrawHRule(*canvas_, pad, panelW - pad - 1, splitY, kHeaderRule);
				cursorY = splitY + 1;
			}
			const int bodyY = cursorY + kHeaderBodyGap_;
			CanvasTextDraw::Draw(
				*canvas_,
				bodyBlocks[i],
				kFontSize_,
				pad,
				bodyY,
				innerW,
				h,
				Colors::White,
				true);
			cursorY = bodyY + h;
			drewBody = true;
		}
		const int bodyRuleY = cursorY + kHeaderRuleGap_;
		CanvasTextDraw::DrawHRule(*canvas_, pad, panelW - pad - 1, bodyRuleY, kHeaderRule);
		cursorY = bodyRuleY + 1;
	}

	if (gridH > 0)
	{
		const int gridY = cursorY + kHeaderBodyGap_;
		const int stride = slotSide + kHeaderLineGap_;
		for (std::size_t i = 0; i < slotIcons.size(); ++i)
		{
			const int col = static_cast<int>(i % static_cast<std::size_t>(gridCols));
			const int row = static_cast<int>(i / static_cast<std::size_t>(gridCols));
			BlitHeaderIcon_(
				*canvas_,
				slotIcons[i],
				slotIcons[i]->GetModuleNodeLabel(),
				pad + col * stride,
				gridY + row * stride,
				static_cast<unsigned>(slotSide));
		}
		cursorY = gridY + gridH;
		const int gridRuleY = cursorY + kHeaderRuleGap_;
		CanvasTextDraw::DrawHRule(*canvas_, pad, panelW - pad - 1, gridRuleY, kHeaderRule);
		cursorY = gridRuleY + 1;
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
		CanvasTextDraw::DrawHRule(*canvas_, pad, panelW - pad - 1, uniqueRuleY, kHeaderRule);
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
			CanvasTextDraw::DrawHRule(*canvas_, pad, panelW - pad - 1, splitRuleY, kHeaderRule);
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

// ————————————————————————————————————————————————————
// Ultra：顶区图标和标题每帧染色
// ————————————————————————————————————————————————————
void NodeInfoPanel::TickHeaderRainbow_()
{
	if (canvas_ == nullptr)
	{
		return;
	}
	if (!headerRainbowBits_.has_value() && headerRainbowBoxes_.empty())
	{
		return;
	}

	// 和场上 SubmitIcon 同一套切帧：1 秒 16 帧，相位跟着 BakeRainbowSheet。
	constexpr unsigned kFrames = ModuleNode_Ultra::kRainbowFrames;
	const float cycle = std::fmod(TimeCodex::Get().GetTotalTime(), 1.0f);
	const unsigned frame = static_cast<unsigned>(cycle * static_cast<float>(kFrames)) % kFrames;
	const float phase = IconAtlas::kRainbowPeriod * static_cast<float>(frame) / static_cast<float>(kFrames);

	if (headerRainbowBits_.has_value() && headerIconSide_ > 0)
	{
		Canvas src{
			IModuleNode::kVisualSize,
			IModuleNode::kVisualSize,
			Canvas::Empty
		};
		src.Clear(Colors::None);
		IconAtlas::BlitIconRainbow(src, *headerRainbowBits_, phase);
		CanvasPixelDraw::BlitNearestCentered(
			*canvas_,
			src,
			headerIconX_,
			headerIconY_,
			static_cast<unsigned>(headerIconSide_),
			static_cast<unsigned>(headerIconSide_));
	}

	// 跑马灯已经先把超宽的字画回白底。这里只染标题和类型名。
	for (const HeaderRainbowBox_& box : headerRainbowBoxes_)
	{
		TintDiagonalRainbow_(*canvas_, box.x, box.y, box.w, box.h, phase);
	}
	canvas_->NotifyPixelsChanged();
}

// 基础：超宽单行滚动。奥义的彩虹染色在这之后，所以先把字画回白底。
void NodeInfoPanel::TickMarquee_()
{
	if (marquees_.empty() || canvas_ == nullptr)
	{
		return;
	}

	float dt = TimeCodex::Get().GetUnscaledDeltaTime();
	if (dt < 0.0f)
	{
		dt = 0.0f;
	}
	if (dt > 0.05f)
	{
		dt = 0.05f;
	}

	for (HeaderMarqueeLine_& line : marquees_)
	{
		const float maxShift = line.textW - static_cast<float>(line.boxW);
		const float loopShift = line.textW + kMarqueeGapPx_;
		switch (line.phase)
		{
		case HeaderMarqueeLine_::Phase::HoldStart:
			line.holdRemain -= dt;
			if (line.holdRemain <= 0.0f)
			{
				line.phase = HeaderMarqueeLine_::Phase::ScrollToEnd;
			}
			break;
		case HeaderMarqueeLine_::Phase::ScrollToEnd:
			line.offsetPx -= kMarqueePxPerSec_ * dt;
			if (line.offsetPx <= -maxShift)
			{
				line.offsetPx = -maxShift;
				line.phase = HeaderMarqueeLine_::Phase::HoldEnd;
				line.holdRemain = kMarqueeHoldSec_;
			}
			break;
		case HeaderMarqueeLine_::Phase::HoldEnd:
			line.holdRemain -= dt;
			if (line.holdRemain <= 0.0f)
			{
				line.phase = HeaderMarqueeLine_::Phase::ScrollGap;
			}
			break;
		case HeaderMarqueeLine_::Phase::ScrollGap:
			line.offsetPx -= kMarqueePxPerSec_ * dt;
			if (line.offsetPx <= -loopShift)
			{
				line.offsetPx = 0.0f;
				line.phase = HeaderMarqueeLine_::Phase::HoldStart;
				line.holdRemain = kMarqueeHoldSec_;
			}
			break;
		}
		PaintMarqueeLine_(line);
	}

	canvas_->NotifyPixelsChanged();
}

void NodeInfoPanel::PaintMarqueeLine_(const HeaderMarqueeLine_& line)
{
	if (canvas_ == nullptr || line.boxW <= 0 || line.boxH <= 0)
	{
		return;
	}

	CanvasPixelDraw::FillRect(
		*canvas_,
		static_cast<unsigned>(line.boxX),
		static_cast<unsigned>(line.boxY),
		static_cast<unsigned>(line.boxX + line.boxW - 1),
		static_cast<unsigned>(line.boxY + line.boxH - 1),
		kPanelBg);

	const float copies[2]{
		line.offsetPx,
		line.offsetPx + line.textW + kMarqueeGapPx_
	};
	for (float off : copies)
	{
		const int ox = static_cast<int>(std::floor(off));
		if (line.paintFusionKindBack)
		{
			FillFusionKindBacks_(
				*canvas_,
				line.fusionKinds,
				kFontSize_,
				line.boxX,
				line.boxY,
				line.boxH,
				line.boxX + line.boxW,
				ox);
		}
		CanvasTextDraw::Draw(
			*canvas_,
			line.text,
			kFontSize_,
			line.boxX,
			line.boxY,
			line.boxW,
			line.boxH,
			line.color,
			false,
			line.spans,
			0,
			DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
			off);
	}
}