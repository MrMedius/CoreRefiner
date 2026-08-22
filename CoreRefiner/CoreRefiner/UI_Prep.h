#pragma once
#include "ModuleWorkbench.h"

class Window;

/**
 * @brief 战备阶段 UI：布局编辑 + Shop / Warehouse。不拥有 ModuleWorkbench。
 */
class UI_Prep
{
public:
	explicit UI_Prep(ModuleWorkbench& workbench)
		:
		workbench_(workbench)
	{}
	~UI_Prep() = default;

	void SetHostWindow(Window* window) noexcept { hostWindow_ = window; }

	void BeginLayoutEdit() { workbench_.BeginLayoutEdit(); }

	void EndLayoutEdit() { workbench_.EndLayoutEdit(); }

	void UpdateLayoutEdit(float dt) { workbench_.UpdateLayoutEdit(dt, hostWindow_); }

	/** @brief 提交战备整页（Field + Shop + 仓 + 编辑器）。 */
	void Submit() { workbench_.SubmitPrep(); }

	[[nodiscard]] bool IsLayoutEditActive() const noexcept { return workbench_.IsLayoutEditActive(); }

private:
	ModuleWorkbench& workbench_;
	Window* hostWindow_{ nullptr };
};
