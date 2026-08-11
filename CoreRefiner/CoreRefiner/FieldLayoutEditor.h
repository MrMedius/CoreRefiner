#pragma once

#include "Canvas2D.h"
#include "ModuleField.h"
#include "ModuleFieldCanvas.h"
#include "ModuleWarehouse.h"
#include "NodeInfoPanel.h"
#include "ScanAssembler.h"
#include "Colors.h"

#include "Win.h"

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

/**
 * @brief Pause-time ModuleField + ModuleWarehouse layout editor.
 */
class FieldLayoutEditor
{
public:
	FieldLayoutEditor() = default;
	~FieldLayoutEditor() = default;

	FieldLayoutEditor(const FieldLayoutEditor&) = delete;
	FieldLayoutEditor& operator=(const FieldLayoutEditor&) = delete;

	void Begin(
		ModuleField& field,
		ModuleWarehouse& warehouse,
		ScanAssembler& assembler,
		ModuleFieldCanvas& canvas,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 combatOrigin);

	/**
	 * @brief Leave layout edit; keep current Field/Warehouse arrangement (no Esc snapshot).
	 */
	void End(ModuleField& field, ModuleFieldCanvas& canvas);

	/**
	 * @brief Esc: restore Begin-time zone + localPos from layoutSnapshot_.
	 */
	void CancelRestore(ModuleField& field);

	void Update(float dt, ModuleField& field, Window* hostWindow);

	void SubmitOverlay();

	[[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
	enum class DragSource_ : unsigned char
	{
		None,
		Field,
		Warehouse,
	};

	enum class DropTarget_ : unsigned char
	{
		None,
		Field,
		Warehouse,
	};

	struct DropEval_
	{
		bool placeable{ false };
		DropTarget_ target{ DropTarget_::None };
	};

	enum class LayoutZone_ : unsigned char
	{
		Field,
		Warehouse,
	};

	struct LayoutSnapshotEntry_
	{
		std::uint32_t id{ 0 };
		LayoutZone_ zone{ LayoutZone_::Field };
		DirectX::XMFLOAT2 localPos{ 0.0f, 0.0f };
	};

	void EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	[[nodiscard]] static std::unique_ptr<Canvas2D> MakeRingCanvas_(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		Color ringColor);

	void Snapshot_(const ModuleField& field);

	[[nodiscard]] IFieldNode* FindNodeById_(
		ModuleField& field,
		std::uint32_t id,
		LayoutZone_& outZone) const noexcept;

	void ApplyFieldOrigin_(ModuleField& field, ModuleFieldCanvas& canvas, DirectX::XMFLOAT3 origin);
	void ClearAllLayoutGhosts_(ModuleField& field);
	[[nodiscard]] DirectX::XMFLOAT2 MouseGame_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT3 OriginForSource_(DragSource_ source) const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 WorldPosOf_(const IFieldNode& node, DirectX::XMFLOAT3 origin) const noexcept;

	[[nodiscard]] bool FieldContainsCircle_(DirectX::XMFLOAT2 worldCenter, float radius) const noexcept;

	[[nodiscard]] DirectX::XMFLOAT2 ClampLocalForNode_(
		DirectX::XMFLOAT2 p,
		float hitRadius) const noexcept;

	[[nodiscard]] bool WouldOverlapOthers_(
		const ModuleField& field,
		const IFieldNode& self,
		DirectX::XMFLOAT2 fieldLocal) const noexcept;

	void SetFreePreview_(IFieldNode& node, DirectX::XMFLOAT2 mouseGame);

	// Green/red drop legality for the current dragged node pose.
	[[nodiscard]] DropEval_ EvalDrop_(const ModuleField& field, const IFieldNode& node) const noexcept;

	void RevertDrag_(ModuleField& field);
	void ResolveRelease_(ModuleField& field);
	
	// If a drag is active, revert like an illegal drop and clear drag pointers.
	void ClearActiveDrag_(ModuleField& field);

	[[nodiscard]] IFieldNode* PickHover_(
		ModuleField& field,
		DirectX::XMFLOAT2 mouseGame,
		DragSource_& outSource) const noexcept;

	void SnapCursorToNode_(
		IFieldNode& node,
		DirectX::XMFLOAT3 origin,
		Window& hostWindow) const noexcept;
	void SyncRingTransform_(IFieldNode& node, DirectX::XMFLOAT3 origin);
	void SyncOneRingTransform_(Canvas2D& ring, IFieldNode& node, DirectX::XMFLOAT3 origin) const;
	[[nodiscard]] Canvas2D* ActiveRing_() const noexcept;

	bool active_{ false };

	DirectX::XMFLOAT3 combatOrigin_{ 200.0f, 200.0f, 0.0f };
	DirectX::XMFLOAT3 editOrigin_{ 0.0f, 0.0f, 0.0f };
	ModuleWarehouse* warehouse_{ nullptr };

	std::vector<LayoutSnapshotEntry_> layoutSnapshot_;
	DirectX::XMFLOAT2 dragStartLocalPos_{ 0.0f, 0.0f };
	DirectX::XMFLOAT3 dragOrigin_{ 0.0f, 0.0f, 0.0f };

	IFieldNode* hover_{ nullptr };
	IFieldNode* dragged_{ nullptr };
	DragSource_ hoverSource_{ DragSource_::None };
	DragSource_ dragSource_{ DragSource_::None };

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
