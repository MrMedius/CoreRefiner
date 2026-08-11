#pragma once

#include "Canvas2D.h"
#include "ModuleField.h"
#include "ModuleFieldCanvas.h"
#include "ModuleWarehouse.h"
#include "NodeInfoPanel.h"
#include "ScanAssembler.h"
#include "Colors.h"

#include "Win.h"

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

	void End(ModuleField& field, ModuleFieldCanvas& canvas);

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

	void EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	[[nodiscard]] static std::unique_ptr<Canvas2D> MakeRingCanvas_(
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		Color ringColor);

	void Snapshot_(const ModuleField& field);
	void ApplyFieldOrigin_(ModuleField& field, ModuleFieldCanvas& canvas, DirectX::XMFLOAT3 origin);
	void ClearAllLayoutGhosts_(ModuleField& field);
	[[nodiscard]] DirectX::XMFLOAT2 MouseGame_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT3 OriginForSource_(DragSource_ source) const noexcept;

	[[nodiscard]] DirectX::XMFLOAT2 ClampLocalForNode_(
		DirectX::XMFLOAT2 p,
		float hitRadius) const noexcept;

	[[nodiscard]] bool WouldOverlapOthers_(
		const ModuleField& field,
		const IFieldNode& self,
		DirectX::XMFLOAT2 candidate) const noexcept;

	/**
	 * @brief Free preview: local = mouseGame - dragOrigin_ (no field clamp).
	 */
	void SetFreePreview_(IFieldNode& node, DirectX::XMFLOAT2 mouseGame);

	void CommitOrRevertDrag_(ModuleField& field, IFieldNode& node);

	/**
	 * @brief Pick nearest hit in Field or Warehouse (game-pixel circles).
	 */
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

	std::vector<DirectX::XMFLOAT2> snapshotLocalPos_;
	DirectX::XMFLOAT2 dragStartLocalPos_{ 0.0f, 0.0f };
	/** @brief Container origin frozen at grab; preview uses mouseGame - dragOrigin_. */
	DirectX::XMFLOAT3 dragOrigin_{ 0.0f, 0.0f, 0.0f };

	IFieldNode* hover_{ nullptr };
	IFieldNode* dragged_{ nullptr };
	DragSource_ hoverSource_{ DragSource_::None };
	DragSource_ dragSource_{ DragSource_::None };

	enum class RingKind_
	{
		Hover,   ///< Idle hover — yellow
		Valid,   ///< Dragging, no overlap — green
		Overlap, ///< Dragging, overlaps another node — red
	};
	RingKind_ ringKind_{ RingKind_::Hover };
	std::unique_ptr<Canvas2D> ringHover_;
	std::unique_ptr<Canvas2D> ringValid_;
	std::unique_ptr<Canvas2D> ringOverlap_;
	NodeInfoPanel infoPanel_;

	static constexpr float kRingPadding_{ 12.0f };
};
