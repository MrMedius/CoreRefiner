#pragma once

#include "Canvas2D.h"
#include "IModuleZone.h"
#include "NodeInfoPanel.h"
#include "Colors.h"

#include "Win.h"

#include <array>
#include <cstdint>
#include <DirectXMath.h>
#include <memory>
#include <vector>

class Window;
class Graphics;

namespace Rgph
{
	class RenderGraph;
}

class ZoneLayoutEditor
{
public:
	ZoneLayoutEditor() = default;
	~ZoneLayoutEditor() = default;

	ZoneLayoutEditor(const ZoneLayoutEditor&) = delete;
	ZoneLayoutEditor& operator=(const ZoneLayoutEditor&) = delete;

	void Begin(std::array<IModuleZone*, ZoneCount()> zones, std::array<DirectX::XMFLOAT3, ZoneCount()> origins, Graphics& gfx, Rgph::RenderGraph& rg, DirectX::XMFLOAT3 combatFieldOrigin);

	void End();

	void CancelRestore();

	void Update(float dt, Window* hostWindow);

	void SubmitOverlay();

	[[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
	static constexpr ZoneId kNoZone_{ ZoneId::Count };

	struct DropEval_
	{
		bool placeable{ false };
		ZoneId target{ kNoZone_ };
	};

	struct LayoutSnapshotEntry_
	{
		std::uint32_t id{ 0 };
		ZoneId zone{ ZoneId::Field };
		DirectX::XMFLOAT2 localPos{ 0.0f, 0.0f };
	};

	void EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	[[nodiscard]] static std::unique_ptr<Canvas2D> MakeRingCanvas_(Graphics& gfx, Rgph::RenderGraph& rg, Color ringColor);

	void Snapshot_();

	[[nodiscard]] IModuleNode* FindNodeById_(std::uint32_t id, ZoneId& outZone) const noexcept;

	void ClearAllLayoutGhosts_();
	[[nodiscard]] DirectX::XMFLOAT2 MouseGame_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT3 OriginForSource_(ZoneId source) const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 WorldPosOf_(const IModuleNode& node, DirectX::XMFLOAT3 origin) const noexcept;
	[[nodiscard]] IModuleZone* ZoneAt_(ZoneId id) const noexcept;

	void SetFreePreview_(IModuleNode& node, DirectX::XMFLOAT2 mouseGame);

	[[nodiscard]] DropEval_ EvalDrop_(const IModuleNode& node) const noexcept;

	void RevertDrag_();
	void ResolveRelease_();
	void ClearActiveDrag_();

	[[nodiscard]] IModuleNode* PickHover_(DirectX::XMFLOAT2 mouseGame, ZoneId& outSource) const noexcept;

	void SnapCursorToNode_(IModuleNode& node, DirectX::XMFLOAT3 origin, Window& hostWindow) const noexcept;
	void SyncRingTransform_(IModuleNode& node, DirectX::XMFLOAT3 origin);
	void SyncOneRingTransform_(Canvas2D& ring, IModuleNode& node, DirectX::XMFLOAT3 origin) const;
	[[nodiscard]] Canvas2D* ActiveRing_() const noexcept;

	bool active_{ false };

	DirectX::XMFLOAT3 combatOrigin_{ 200.0f, 200.0f, 0.0f };
	std::array<IModuleZone*, ZoneCount()> zones_{};

	std::vector<LayoutSnapshotEntry_> layoutSnapshot_;
	DirectX::XMFLOAT2 dragStartLocalPos_{ 0.0f, 0.0f };
	DirectX::XMFLOAT3 dragOrigin_{ 0.0f, 0.0f, 0.0f };

	IModuleNode* hover_{ nullptr };
	IModuleNode* dragged_{ nullptr };
	ZoneId hoverSource_{ kNoZone_ };
	ZoneId dragSource_{ kNoZone_ };

	enum class RingKind_
	{
		Hover,   ///< Idle hover — yellow
		Valid,   ///< Dragging, placeable — green
		Overlap, ///< Dragging, not placeable — red
	};
	RingKind_ ringKind_{ RingKind_::Hover };
	std::unique_ptr<Canvas2D> ringHover_;
	std::unique_ptr<Canvas2D> ringValid_;
	std::unique_ptr<Canvas2D> ringOverlap_;
	NodeInfoPanel infoPanel_;

	static constexpr float kRingPadding_{ 12.0f };
};
