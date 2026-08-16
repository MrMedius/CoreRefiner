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

	/**
	 * @brief Lazily create the canvas and link UI techniques.
	 */
	void Ensure(Graphics& gfx, Rgph::RenderGraph& rg);

	/**
	 * @brief Show copy for @p label anchored near @p anchorGameXY (game pixels, center-ish).
	 */
	void ShowFor(ModuleNodeLabel label, DirectX::XMFLOAT2 anchorGameXY);

	void Hide() noexcept;

	[[nodiscard]] bool IsVisible() const noexcept { return visible_; }

	/** @brief Submit to Chan::ui when visible and ready. */
	void Submit() const;

private:
	void RebuildContent_(ModuleNodeLabel label);
	void SyncPosition_(DirectX::XMFLOAT2 anchorGameXY);

	Graphics* gfx_{ nullptr };
	Rgph::RenderGraph* rg_{ nullptr };
	std::unique_ptr<Canvas2D> canvas_;

	bool visible_{ false };
	std::optional<ModuleNodeLabel> cachedLabel_;
	std::optional<ModuleNodeInfoLanguage> cachedLanguage_;
	unsigned contentW_{ 1u };
	unsigned contentH_{ 1u };

	static constexpr float kMaxWidthPx_{ 320.0f };
	static constexpr float kScreenPad_{ 8.0f };
	static constexpr float kAnchorGap_{ 16.0f };
	static constexpr int kPaddingPx_{ 10 };
	static constexpr float kFontSize_{ 18.0f };
};
