#pragma once

#include "ModuleNodeLabel.h"
#include "ModuleNodeInfoCopy.h"
#include "Colors.h"

#include <cstdint>
#include <DirectXMath.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Canvas2D;
class Graphics;
class IModuleNode;

namespace Rgph
{
	class RenderGraph;
}

class NodeInfoPanel
{
public:
	NodeInfoPanel() = default;
	~NodeInfoPanel() = default;

	NodeInfoPanel(const NodeInfoPanel&) = delete;
	NodeInfoPanel& operator=(const NodeInfoPanel&) = delete;

	enum class Anchor : unsigned char
	{
		Above,
		Below,
	};

	// 固定面板宽度；高度随文案变。Fusion 日/英 Kind 行按这个提前量排。
	static constexpr float kMaxWidthPx_{ 300.0f };

	void Ensure(Graphics& gfx, Rgph::RenderGraph& rg);

	void ShowFor(ModuleNodeLabel label, DirectX::XMFLOAT2 anchorGameXY, Anchor anchor = Anchor::Above, float maxWidthPx = kMaxWidthPx_);
	void ShowFor(const IModuleNode& node, DirectX::XMFLOAT2 anchorGameXY, Anchor anchor = Anchor::Above, float maxWidthPx = kMaxWidthPx_);

	void Hide() noexcept;

	// 超框单行在此每帧滚 offset，不靠 RebuildContent_ 脏标记。
	void Submit();

private:
	struct HeaderMarqueeLine_
	{
		std::string text;
		std::vector<Text::Span> spans;
		std::vector<ModuleNodeKind> fusionKinds;
		Color color{ Colors::White };
		int boxX{ 0 };
		int boxY{ 0 };
		int boxW{ 0 };
		int boxH{ 0 };
		float textW{ 0.0f };
		float offsetPx{ 0.0f };
		float holdRemain{ 0.0f };
		enum class Phase : unsigned char
		{
			HoldStart,
			ScrollToEnd,
			HoldEnd,
			ScrollGap
		};
		Phase phase{ Phase::HoldStart };
		bool paintFusionKindBack{ false };
	};

	void RebuildContent_(
		const ModuleNodeInfoEntry& entry,
		const IModuleNode* node,
		ModuleNodeLabel label);
	void SyncPosition_(DirectX::XMFLOAT2 anchorGameXY);
	void TickMarquee_();
	void PaintMarqueeLine_(const HeaderMarqueeLine_& line);

	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	std::unique_ptr<Canvas2D> canvas_;

	bool visible_{ false };
	std::optional<ModuleNodeLabel> cachedLabel_;
	std::optional<std::uint32_t> cachedInstanceId_;
	std::optional<int> cachedLevel_;
	std::optional<int> cachedBuyPrice_;
	std::optional<Language> cachedLanguage_;
	// 只在悬停奥义时有值。和等级、造价分开记。
	std::optional<std::uint32_t> cachedUltraRevision_;
	Anchor anchor_{ Anchor::Above };
	float maxWidthPx_{ kMaxWidthPx_ };
	unsigned contentW_{ 1u };
	unsigned contentH_{ 1u };
	std::vector<HeaderMarqueeLine_> marquees_;

	static constexpr float kScreenPad_{ 8.0f };
	static constexpr float kAnchorGap_{ 16.0f };
	static constexpr int kPaddingPx_{ 10 };
	static constexpr float kFontSize_{ 18.0f };
	static constexpr int kHeaderLineGap_{ 4 };
	static constexpr int kHeaderIconTextGap_{ 8 };
	static constexpr int kHeaderRuleGap_{ 4 };
	static constexpr int kHeaderBodyGap_{ 6 };
	static constexpr float kMarqueePxPerSec_{ 48.0f };
	static constexpr float kMarqueeHoldSec_{ 0.75f };
	static constexpr float kMarqueeGapPx_{ 40.0f };
};