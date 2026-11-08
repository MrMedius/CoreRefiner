#pragma once
#include "IconAtlas.h"
#include "ModuleField.h"
#include "ModuleShop.h"
#include "ModuleWarehouse.h"
#include "ScanAssembler.h"
#include "ZoneLayoutEditor.h"

#include <cstdint>
#include <DirectXMath.h>
#include <functional>
#include <memory>
#include <string>

class UI_SellConfirm;
class UI_UltraRename;
class UI_UltraRedraw;

class Attack;
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

	// 区。场、仓、商店和扫描还在各自的类里。
	[[nodiscard]] ModuleField& GetField() noexcept { return field_; }
	[[nodiscard]] const ModuleField& GetField() const noexcept { return field_; }

	[[nodiscard]] ModuleWarehouse& GetWarehouse() noexcept { return warehouse_; }
	[[nodiscard]] const ModuleWarehouse& GetWarehouse() const noexcept { return warehouse_; }

	[[nodiscard]] ModuleShop& GetShop() noexcept { return shop_; }
	[[nodiscard]] const ModuleShop& GetShop() const noexcept { return shop_; }

	[[nodiscard]] ScanAssembler& GetAssembler() noexcept { return assembler_; }
	[[nodiscard]] const ScanAssembler& GetAssembler() const noexcept { return assembler_; }

	// 布局。战斗时场贴右下；整理时右列是场、仓和战斗按钮，左边是商店。
	[[nodiscard]] DirectX::XMFLOAT3 GetCombatFieldOrigin() const noexcept { return combatFieldOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetLayoutFieldOrigin() const noexcept { return layoutFieldOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetWarehouseOrigin() const noexcept { return warehouseOrigin_; }
	[[nodiscard]] DirectX::XMFLOAT3 GetShopOrigin() const noexcept { return shopOrigin_; }

	// 演示摆放。新开一局把场和仓放回演示，商店重进货。
	void Reset();

	// 战斗按钮。没 Core 时灰掉，有 Core 时显示下一波。
	// 点战斗按钮且场上有 Core 时调用。
	void SetOnFight(std::function<void()> cb) { onFight_ = std::move(cb); }
	// 刷新战斗按钮文案；波次不变则不重绘。
	void SetNextWave(int wave);
	// 按当前语言与 Field 是否有 Core，刷新战斗键文案和灰态。
	void RefreshFightLabel();

	// 整理。拖放和商店按钮在编辑器里，这里只开关这一页。
	// 战备整理是否开着。
	[[nodiscard]] bool IsLayoutEditActive() const noexcept { return layoutEditor_.IsActive(); }
	// 进入战备整理。
	void BeginLayoutEdit();
	// 离开整理，并关掉叠加界面。
	void EndLayoutEdit();
	// 整理页这一帧。叠加界面开着时只更新它。
	void UpdateLayoutEdit(float dt, Window* hostWindow);

	// 叠加界面。卖出确认、更名、重绘打开时挡住整理。
	// 三者里是否有一个正打开。
	[[nodiscard]] bool IsOverlayOpen() const noexcept;
	// 关掉正在打开的叠加界面。不改名字、图案，也不卖出。
	void CloseOverlay() noexcept;
	// 语言变了就刷新叠加界面上的按钮文案。
	void RefreshOverlayLabels();

	// 提交。战斗只交场；战备页再交仓、商店、编辑器和按钮。
	void SubmitField();
	void SubmitPrep();

	// 战斗。冷却、扫描和开火。
	void Update(float dt, AttackManager* attackManager);
	// 仓内每个占用节点对这颗弹调用 ApplyWarehouseBonus。
	void ApplyWarehouseBonuses(Attack& atk);

private:
	// 战斗按钮的逻辑宽高。布局和按钮共用。
	static constexpr float kFightBtnW = 240.0f;
	static constexpr float kFightBtnH = 48.0f;

	// 布局。算出战备页上商店、场、仓和战斗按钮的原点。
	void ComputeLayout_() noexcept;
	// 演示摆放。场上的开局棋子。
	void PlaceDemoField_();
	// 演示摆放。仓库的开局棋子，含一颗留着测试的奥义。
	void PlaceDemoWarehouse_();
	// 战斗按钮。建按钮、接上点击，并画出第 1 波。
	void InitFightButton_();
	// 叠加界面。卖出确认、更名和重绘都从编辑器的请求打开。
	void BindOverlays_();
	// 按编号找到商店里的那颗奥义并改名。找不到就不改。
	void ApplyRename_(std::uint32_t instanceId, std::string name);
	// 按编号找到商店里的那颗奥义，写入新图案并重新烘焙。找不到就不改。
	void ApplyRedraw_(std::uint32_t instanceId, const IconAtlas::IconBits& bits);
	// 战斗。把还在扫的波交给场画出来。
	void SyncFieldWaves_();

	// 建画布和按钮用的设备和渲染图。
	Graphics& gfx_;
	Rgph::RenderGraph& rg_;

	// 布局。
	// 战斗时场的原点，贴在右下。
	DirectX::XMFLOAT3 combatFieldOrigin_{ 0.0f, 0.0f, 0.0f };
	// 整理时场的原点。
	DirectX::XMFLOAT3 layoutFieldOrigin_{ 0.0f, 0.0f, 0.0f };
	// 仓库原点。
	DirectX::XMFLOAT3 warehouseOrigin_{ 0.0f, 0.0f, 0.0f };
	// 商店原点。
	DirectX::XMFLOAT3 shopOrigin_{ 0.0f, 0.0f, 0.0f };
	// 战斗按钮中心。
	DirectX::XMFLOAT2 fightBtnCenter_{ 0.0f, 0.0f };

	// 区。
	ModuleField field_;
	ModuleWarehouse warehouse_;
	ModuleShop shop_;
	ScanAssembler assembler_;
	// 整理。拖放在编辑器里。
	ZoneLayoutEditor layoutEditor_;
	// 叠加界面。编号留到确定后再找 Node。
	std::unique_ptr<UI_SellConfirm> sellConfirm_;
	// 更名界面。打开时和卖出确认一样挡住整理。
	std::unique_ptr<UI_UltraRename> rename_;
	// 重绘界面。打开时和卖出确认一样挡住整理。
	std::unique_ptr<UI_UltraRedraw> redraw_;
	// 正在等确认卖掉的 Node 编号。
	std::uint32_t pendingSellId_{ 0 };
	// 正在更名的奥义编号。确定时再按这个编号去找。
	std::uint32_t pendingRenameId_{ 0 };
	// 正在重绘的奥义编号。确定时再按这个编号去找。
	std::uint32_t pendingRedrawId_{ 0 };

	// 战斗按钮。波次没变就不重画文案。
	std::unique_ptr<Ui::UiRoot> uiRoot_;
	std::unique_ptr<Ui::ButtonCanvasComponent> fightBtn_;
	std::function<void()> onFight_{};
	// 按钮上已经画过的波次。
	int paintedWave_{ -1 };
};
