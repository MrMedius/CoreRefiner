#pragma once

#include "Graphics.h"
#include "RenderGraph.h"
#include "ModuleWorkbench.h"
#include "AttackManager.h"

class Window;

class UI_Game
{
public:
	UI_Game(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		workbench_(gfx, rg)
	{}
	~UI_Game() = default;

	void SetAttackManager(AttackManager* manager) noexcept { attackManager_ = manager; }

	void SetHostWindow(Window* window) noexcept { hostWindow_ = window; }

	void Reset() { workbench_.Reset(); }

	void Update(float dt) { workbench_.Update(dt, attackManager_); }

	void BeginLayoutEdit() { workbench_.BeginLayoutEdit(); }

	void EndLayoutEdit() { workbench_.EndLayoutEdit(); }

	void UpdateLayoutEdit(float dt) { workbench_.UpdateLayoutEdit(dt, hostWindow_); }

	void Submit() { workbench_.Submit(); }

	[[nodiscard]] ModuleField& GetField() noexcept { return workbench_.GetField(); }
	[[nodiscard]] const ModuleField& GetField() const noexcept { return workbench_.GetField(); }
	[[nodiscard]] ModuleWarehouse& GetWarehouse() noexcept { return workbench_.GetWarehouse(); }
	[[nodiscard]] const ModuleWarehouse& GetWarehouse() const noexcept { return workbench_.GetWarehouse(); }
	[[nodiscard]] ScanAssembler& GetAssembler() noexcept { return workbench_.GetAssembler(); }
	[[nodiscard]] ModuleWorkbench& GetWorkbench() noexcept { return workbench_; }
	[[nodiscard]] const ModuleWorkbench& GetWorkbench() const noexcept { return workbench_; }

private:
	AttackManager* attackManager_{ nullptr };
	Window* hostWindow_{ nullptr };
	ModuleWorkbench workbench_;
};
