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

class UI_SellConfirm
{
public:
	UI_SellConfirm(Graphics& gfx, Rgph::RenderGraph& rg);
	~UI_SellConfirm() = default;

	UI_SellConfirm(const UI_SellConfirm&) = delete;
	UI_SellConfirm& operator=(const UI_SellConfirm&) = delete;

	void SetOnConfirm(std::function<void()> cb);
	void SetOnExit(std::function<void()> cb);

	void Show(int price);
	void Hide() noexcept;
	[[nodiscard]] bool IsOpen() const noexcept;

	void RefreshLabels();

	void Update(float dt);
	void Submit();

private:
	void PaintPrompt_();

	std::unique_ptr<Canvas2D> bg_;
	std::unique_ptr<Canvas2D> prompt_;
	std::unique_ptr<Ui::UiRoot> uiRoot_;
	std::unique_ptr<Ui::ButtonCanvasComponent> btnConfirm_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnExit_{};

	std::function<void()> onConfirm_{};
	std::function<void()> onExit_{};

	int price_{ 0 };
	bool open_{ false };
};
