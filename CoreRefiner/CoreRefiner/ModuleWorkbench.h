#pragma once
#include "ModuleField.h"
#include "ModuleShop.h"
#include "ModuleWarehouse.h"
#include "ScanAssembler.h"
#include "ZoneLayoutEditor.h"
#include <DirectXMath.h>

class AttackManager;
class Graphics;
class Window;

namespace Rgph
{
	class RenderGraph;
}

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

	[[nodiscard]] ModuleShop& GetShop() noexcept { return shop_; }
	[[nodiscard]] const ModuleShop& GetShop() const noexcept { return shop_; }

	[[nodiscard]] ScanAssembler& GetAssembler() noexcept { return assembler_; }
	[[nodiscard]] const ScanAssembler& GetAssembler() const noexcept { return assembler_; }

	[[nodiscard]] bool IsLayoutEditActive() const noexcept { return layoutEditor_.IsActive(); }

	[[nodiscard]] DirectX::XMFLOAT3 GetCombatFieldOrigin() const noexcept { return combatFieldOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetLayoutFieldOrigin() const noexcept { return layoutFieldOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetWarehouseOrigin() const noexcept { return warehouseOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetShopOrigin() const noexcept { return shopOrigin_; }

	void Reset();

	void Update(float dt, AttackManager* attackManager);

	void BeginLayoutEdit();
	void EndLayoutEdit();
	void UpdateLayoutEdit(float dt, Window* hostWindow);

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
	DirectX::XMFLOAT3 shopOrigin_{ 0.0f, 0.0f, 0.0f };

	ModuleField field_;
	ModuleWarehouse warehouse_;
	ModuleShop shop_;
	ScanAssembler assembler_;
	ZoneLayoutEditor layoutEditor_;
};
