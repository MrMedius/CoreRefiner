#pragma once
#include "Graphics.h"

#include "ButtonCanvasComponent.h"
#include "UiRoot.h"

#include "Channels.h"

class UI_Title
{
public:
	UI_Title(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		constexpr Ui::FocusHandle kFocusBtnA = 501u;
		constexpr Ui::FocusHandle kFocusBtnB = 502u;
		constexpr Ui::FocusHandle kFocusBtnC = 503u;

		constexpr unsigned kCanvasLogicalW = 380u;
		constexpr unsigned kCanvasLogicalH = 100u;

		btnA_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 501u, 380u, 100u);
		btnB_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 502u, 380u, 100u);
		btnC_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 503u, 380u, 100u);

		btnA_->SetLayoutLogicalCenterSize(390.0f, 350.0f, 380.0f, 100.0f);
		btnB_->SetLayoutLogicalCenterSize(890.0f, 350.0f, 380.0f, 100.0f);
		btnC_->SetLayoutLogicalCenterSize(1390.0f, 350.0f, 380.0f, 100.0f);

		btnA_->Button().SetLabel("Btn A | clicks 0");
		btnB_->Button().SetLabel("Btn B | clicks 0");
		btnC_->Button().SetLabel("Btn C | clicks 0");

		btnA_->Button().SetOnClick([this] { ++clicksA_;	btnA_->Button().SetLabel("Btn A | clicks " + std::to_string(clicksA_)); });
		btnB_->Button().SetOnClick([this] { ++clicksB_;	btnB_->Button().SetLabel("Btn B | clicks " + std::to_string(clicksB_));	});
		btnC_->Button().SetOnClick([this] { ++clicksC_;	btnC_->Button().SetLabel("Btn C | clicks " + std::to_string(clicksC_));	});

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
		uiRoot.TickAfterInput();
	}

	void Submit(void)
	{
		uiRoot.Submit(Chan::ui);
	}

private:
	std::unique_ptr<Ui::ButtonCanvasComponent> btnA_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnC_{};

	unsigned clicksA_ = 0u;
	unsigned clicksB_ = 0u;
	unsigned clicksC_ = 0u;

	Ui::UiRoot uiRoot;
};