#pragma once
#include "ModuleWorkbench.h"

#include <functional>

class Window;

class UI_Prep
{
public:
	explicit UI_Prep(ModuleWorkbench& workbench)
		:
		workbench_(workbench)
	{}

	void SetHostWindow(Window* window) noexcept { hostWindow_ = window; }

	void SetOnFight(std::function<void()> cb) { workbench_.SetOnFight(std::move(cb)); }

	void SetNextWave(int wave) { workbench_.SetNextWave(wave); }

	void BeginLayoutEdit() { workbench_.BeginLayoutEdit(); }

	void EndLayoutEdit() { workbench_.EndLayoutEdit(); }

	void Update(float dt) { workbench_.UpdateLayoutEdit(dt, hostWindow_); }

	void Submit() { workbench_.SubmitPrep(); }

	[[nodiscard]] bool IsLayoutEditActive() const noexcept { return workbench_.IsLayoutEditActive(); }

private:
	ModuleWorkbench& workbench_;
	Window* hostWindow_{ nullptr };
};
