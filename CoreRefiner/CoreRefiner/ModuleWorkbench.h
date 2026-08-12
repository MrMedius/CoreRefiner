#pragma once

#include "FieldLayoutEditor.h"
#include "ModuleField.h"
#include "ModuleWarehouse.h"
#include "ScanAssembler.h"

#include <DirectXMath.h>

class AttackManager;
class Graphics;
class Window;

namespace Rgph
{
	class RenderGraph;
}

/**
 * @brief Owns ModuleField / ModuleWarehouse / ScanAssembler / FieldLayoutEditor.
 * @note UI_Game should not own zone instances directly; route through this hub.
 */
class ModuleWorkbench
{
public:
	ModuleWorkbench(Graphics& gfx, Rgph::RenderGraph& rg);
	~ModuleWorkbench() = default;

	ModuleWorkbench(const ModuleWorkbench&) = delete;
	ModuleWorkbench& operator=(const ModuleWorkbench&) = delete;

	[[nodiscard]] ModuleField& GetField() noexcept { return field_; }
	[[nodiscard]] const ModuleField& GetField() const noexcept { return field_; }

	[[nodiscard]] ModuleWarehouse& GetWarehouse() noexcept { return warehouse_; }
	[[nodiscard]] const ModuleWarehouse& GetWarehouse() const noexcept { return warehouse_; }

	[[nodiscard]] ScanAssembler& GetAssembler() noexcept { return assembler_; }
	[[nodiscard]] const ScanAssembler& GetAssembler() const noexcept { return assembler_; }

	[[nodiscard]] bool IsLayoutEditActive() const noexcept { return layoutEditor_.IsActive(); }

	[[nodiscard]] DirectX::XMFLOAT3 GetCombatFieldOrigin() const noexcept { return combatFieldOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetLayoutFieldOrigin() const noexcept { return layoutFieldOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetWarehouseOrigin() const noexcept { return warehouseOrigin_; }

	/** @brief Clear scan sessions, node cooldowns, and field ring draw. */
	void Reset();

	/**
	 * @brief Combat tick: cooldowns, scan assemble, fire batches, field waves.
	 */
	void Update(float dt, AttackManager* attackManager);

	void BeginLayoutEdit();
	void EndLayoutEdit();
	void UpdateLayoutEdit(float dt, Window* hostWindow);

	/**
	 * @brief Submit order: all backgrounds → all nodes → layout overlay when editing.
	 */
	void Submit();

private:
	void PlaceDemoField_();
	void PlaceDemoWarehouse_();
	void SyncFieldWaves_();

	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	DirectX::XMFLOAT3 combatFieldOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 layoutFieldOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };

	ModuleField field_;
	ModuleWarehouse warehouse_;
	ScanAssembler assembler_;
	FieldLayoutEditor layoutEditor_;
};
