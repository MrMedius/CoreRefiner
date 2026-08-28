#pragma once
#include "Canvas2D.h"

#include "UiRoot.h"
#include "ButtonCanvasComponent.h"
#include "SliderCanvasComponent.h"
#include "StepperCanvasComponent.h"
#include "ToggleCanvasComponent.h"

#include <array>
#include <functional>
#include <memory>

namespace Rgph
{
	class RenderGraph;
}

class UI_Setting
{
public:
	UI_Setting(Graphics& gfx, Rgph::RenderGraph& rg);
	~UI_Setting() = default;

	UI_Setting(const UI_Setting&) = delete;
	UI_Setting& operator=(const UI_Setting&) = delete;

	void SetOnBack(std::function<void()> cb);
	void SetOnFullscreenChanged(std::function<void(bool)> cb);
	void SetOnWindowSizeIndex(std::function<void(int)> cb);

	void Show();
	void Hide() noexcept;
	[[nodiscard]] bool IsOpen() const noexcept;

	void RefreshLabels();

	void Update(float dt);
	void Submit();

private:
	void SyncFromCodex_();
	void ApplySoundToDevice_() const;
	void RefreshLanguageStepperLabel_();
	void RefreshWindowStepperLabel_();

	std::unique_ptr<Canvas2D> bg_;
	std::unique_ptr<Ui::UiRoot> uiRoot_;

	std::unique_ptr<Ui::ButtonCanvasComponent> title_{};
	std::array<std::unique_ptr<Ui::ButtonCanvasComponent>, 7> rowLabels_{};

	std::unique_ptr<Ui::StepperCanvasComponent> stepperLanguage_{};
	
	std::unique_ptr<Ui::SliderCanvasComponent> sliderMaster_{};
	std::unique_ptr<Ui::SliderCanvasComponent> sliderBgm_{};
	std::unique_ptr<Ui::SliderCanvasComponent> sliderSe_{};
	std::unique_ptr<Ui::ToggleCanvasComponent> toggleMute_{};
	
	std::unique_ptr<Ui::ToggleCanvasComponent> toggleFullscreen_{};
	std::function<void(bool)> onFullscreenChanged_{};

	std::unique_ptr<Ui::StepperCanvasComponent> stepperWindow_{};
	std::function<void(int)> onWindowSizeIndex_{};

	std::unique_ptr<Ui::ButtonCanvasComponent> btnBack_{};
	std::function<void()> onBack_{};

	bool open_{ false };
};