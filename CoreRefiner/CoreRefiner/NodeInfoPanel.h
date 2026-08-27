#pragma once

#include "ModuleNodeLabel.h"
#include "ModuleNodeInfoCopy.h"

#include <DirectXMath.h>
#include <memory>
#include <optional>

class Canvas2D;
class Graphics;

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

	static constexpr float kMaxWidthPx_{ 320.0f };

	void Ensure(Graphics& gfx, Rgph::RenderGraph& rg);

	void ShowFor(ModuleNodeLabel label, DirectX::XMFLOAT2 anchorGameXY, Anchor anchor = Anchor::Above, float maxWidthPx = kMaxWidthPx_);

	void Hide() noexcept;

	void Submit() const;

private:
	void RebuildContent_(ModuleNodeLabel label);
	void SyncPosition_(DirectX::XMFLOAT2 anchorGameXY);

	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	std::unique_ptr<Canvas2D> canvas_;

	bool visible_{ false };
	std::optional<ModuleNodeLabel> cachedLabel_;
	std::optional<Language> cachedLanguage_;
	Anchor anchor_{ Anchor::Above };
	float maxWidthPx_{ kMaxWidthPx_ };
	unsigned contentW_{ 1u };
	unsigned contentH_{ 1u };

	static constexpr float kScreenPad_{ 8.0f };
	static constexpr float kAnchorGap_{ 16.0f };
	static constexpr int kPaddingPx_{ 10 };
	static constexpr float kFontSize_{ 18.0f };
};
