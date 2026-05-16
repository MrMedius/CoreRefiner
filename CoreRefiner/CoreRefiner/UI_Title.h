#pragma once
#include "Graphics.h"

#include "ButtonCanvasComponent.h"
#include "UiRoot.h"

#include "Channels.h"

#include <functional>

class UI_Title
{
public:
	UI_Title(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		unsigned centerX = SCREEN_WIDTH / 2u;
		unsigned centerY = SCREEN_HEIGHT / 2u;
		unsigned spacingY = SCREEN_HEIGHT / 20u;

		unsigned btnWidth = SCREEN_WIDTH / 10u;
		unsigned btnHeight = SCREEN_HEIGHT / 15u;

		btnA_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 501u, centerX, centerY + spacingY * 2u, btnWidth, btnHeight);
		btnB_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 502u, centerX, centerY + spacingY * 4u, btnWidth, btnHeight);
		btnC_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 503u, centerX, centerY + spacingY * 6u, btnWidth, btnHeight);

		btnA_->Button().SetLabel("New Game");
		btnB_->Button().SetLabel("Settings");
		btnC_->Button().SetLabel("Quit");

		btnA_->Button().SetOnClick([this] {	if (onNewGame_)	onNewGame_();});
		btnB_->Button().SetOnClick([this] { btnB_->Button().SetLabel("Btn B | clicks 1"); });
		btnC_->Button().SetOnClick([this] { PostQuitMessage(0); });

		uiRoot.Clear();
		btnA_->RegisterTo(uiRoot);
		btnB_->RegisterTo(uiRoot);
		btnC_->RegisterTo(uiRoot);
		uiRoot.RebuildTabOrderFromSlots();
		uiRoot.InitLinkTechniques(rg);
	}
	~UI_Title() = default;

	void Update(float dt)
	{
		uiRoot.UpdateAfterInput();
	}

	void Submit(void)
	{
		uiRoot.Submit(Chan::ui);
	}

	void SetOnNewGame(std::function<void()> cb) { onNewGame_ = std::move(cb); }

private:
	Ui::UiRoot uiRoot;
	std::unique_ptr<Ui::ButtonCanvasComponent> btnA_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnC_{};

	std::function<void()> onNewGame_{};
};