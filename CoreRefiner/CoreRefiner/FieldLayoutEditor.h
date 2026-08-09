#pragma once

#include "Canvas2D.h"
#include "ModuleField.h"
#include "ModuleFieldCanvas.h"
#include "ScanAssembler.h"

#include "Win.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

class FieldLayoutEditor
{
public:
	FieldLayoutEditor() = default;
	~FieldLayoutEditor() = default;

	FieldLayoutEditor(const FieldLayoutEditor&) = delete;
	FieldLayoutEditor& operator=(const FieldLayoutEditor&) = delete;

	void Begin(
		ModuleField& field,
		ScanAssembler& assembler,
		ModuleFieldCanvas& canvas,
		Graphics& gfx,
		Rgph::RenderGraph& rg,
		DirectX::XMFLOAT3 combatOrigin);

	void End(ModuleField& field, ModuleFieldCanvas& canvas);

	void CancelRestore(ModuleField& field);

	void Update(float dt, ModuleField& field, HWND hostHwnd);

	void SubmitOverlay();

	[[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
	void EnsureRingVisual_(Graphics& gfx, Rgph::RenderGraph& rg);
	void Snapshot_(const ModuleField& field);
	void ApplyFieldOrigin_(ModuleField& field, ModuleFieldCanvas& canvas, DirectX::XMFLOAT3 origin);
	[[nodiscard]] DirectX::XMFLOAT2 MouseToLocal_() const noexcept;
	[[nodiscard]] DirectX::XMFLOAT2 ClampLocal_(DirectX::XMFLOAT2 p) const noexcept;
	[[nodiscard]] IFieldNode* PickHover_(ModuleField& field, DirectX::XMFLOAT2 mouseLocal) const noexcept;
	void SnapCursorToNode_(IFieldNode& node, HWND hostHwnd) const noexcept;
	void SyncRingTransform_(IFieldNode& node);

	bool active_{ false };
	DirectX::XMFLOAT3 combatOrigin_{ 200.0f, 200.0f, 0.0f };
	DirectX::XMFLOAT3 editOrigin_{ 0.0f, 0.0f, 0.0f };
	std::vector<DirectX::XMFLOAT2> snapshotLocalPos_;
	IFieldNode* hover_{ nullptr };
	IFieldNode* dragged_{ nullptr };
	std::unique_ptr<Canvas2D> ring_;
	static constexpr float kRingPadding_{ 8.0f };
};
