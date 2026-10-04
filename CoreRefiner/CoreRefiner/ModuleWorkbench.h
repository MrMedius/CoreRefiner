#pragma once
#include "ModuleField.h"
#include "ModuleShop.h"
#include "ModuleWarehouse.h"
#include "ScanAssembler.h"
#include "ZoneLayoutEditor.h"

#include <DirectXMath.h>
#include <functional>
#include <memory>

class AttackManager;
class Graphics;
class Window;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class ButtonCanvasComponent;
	class UiRoot;
}

class ModuleWorkbench
{
public:
	ModuleWorkbench(Graphics& gfx, Rgph::RenderGraph& rg);
	~ModuleWorkbench();

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

	void SetOnFight(std::function<void()> cb) { onFight_ = std::move(cb); }
	// 刷新战斗按钮文案；波次不变则不重绘。
	void SetNextWave(int wave);
	// 按当前语言与 Field 是否有 Core，刷新战斗键文案和灰态
	void RefreshFightLabel();

	// 新开一局：Field 回到演示布局，仓清空，商店重进货并清刷新次数。
	void Reset();

	void Update(float dt, AttackManager* attackManager);

	void BeginLayoutEdit();
	void EndLayoutEdit();
	void UpdateLayoutEdit(float dt, Window* hostWindow);

	// 战斗叠层：只交 Field 底板与棋子。
	void SubmitField();
	// 战备页：Field + Shop + Warehouse + 编辑器 + 战斗按钮。
	void SubmitPrep();

private:
	static constexpr float kFightBtnW = 240.0f;
	static constexpr float kFightBtnH = 48.0f;

	void PlaceDemoField_();
	void PlaceDemoWarehouse_();
	void SyncFieldWaves_();
	void InitFightButton_();
	// 战备布局：左 Shop、右列 Field / Warehouse / 战斗按钮，统一缝隙。
	void ComputeLayout_() noexcept;

	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	DirectX::XMFLOAT3 combatFieldOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 layoutFieldOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 shopOrigin_{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT2 fightBtnCenter_{ 0.0f, 0.0f };

	ModuleField field_;
	ModuleWarehouse warehouse_;
	ModuleShop shop_;
	ScanAssembler assembler_;
	ZoneLayoutEditor layoutEditor_;

	std::unique_ptr<Ui::UiRoot> uiRoot_;
	std::unique_ptr<Ui::ButtonCanvasComponent> fightBtn_;
	std::function<void()> onFight_{};
	int paintedWave_{ -1 };
};
