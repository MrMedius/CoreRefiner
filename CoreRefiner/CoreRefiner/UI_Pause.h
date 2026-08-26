#pragma once

#include "ButtonCanvasComponent.h"
#include "Canvas2D.h"
#include "Graphics.h"
#include "UiRoot.h"

#include <functional>
#include <memory>

namespace Rgph
{
	class RenderGraph;
}

class UI_Pause
{
public:
	UI_Pause(Graphics& gfx, Rgph::RenderGraph& rg);
	~UI_Pause() = default;

	UI_Pause(const UI_Pause&) = delete;
	UI_Pause& operator=(const UI_Pause&) = delete;

	void SetOnContinue(std::function<void()> cb);
	void SetOnQuit(std::function<void()> cb);

	void Show() noexcept;
	void Hide() noexcept;
	[[nodiscard]] bool IsOpen() const noexcept;

	void Update(float dt);
	void Submit();

private:
	std::unique_ptr<Canvas2D> bg_;
	std::unique_ptr<Ui::UiRoot> uiRoot_;
	std::unique_ptr<Ui::ButtonCanvasComponent> btnContinue_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnQuit_{};

	std::function<void()> onContinue_{};
	std::function<void()> onQuit_{};

	bool open_{ false };
};
