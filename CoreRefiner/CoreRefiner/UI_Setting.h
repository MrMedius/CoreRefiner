#pragma once
#include "Graphics.h"
#include "ButtonCanvasComponent.h"
#include "UiRoot.h"
#include "Canvas2D.h"
#include "TextCodex.h"

#include "Channels.h"

#include <memory>
#include <functional>

class UI_Setting
{
public:
	UI_Setting(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		// Center of the screen
		unsigned centerX = SCREEN_WIDTH / 2u;
		unsigned centerY = SCREEN_HEIGHT / 2u;

		// Title
		{
			unsigned titleWidth = SCREEN_WIDTH / 2u;
			unsigned titleHeight = SCREEN_HEIGHT / 3u;
			titleCanvas = std::make_unique<Canvas2D>(gfx, titleWidth, titleHeight);
			titleCanvas->SetPosition(DirectX::XMFLOAT3{ centerX, centerY / 2u, 0.0f });
			titleCanvas->SetScale(DirectX::XMFLOAT3{ titleWidth, titleHeight, 1.0f });

			std::string Title = "CORE REFINER";
			auto ctx = TextCodex::Get().BeginDraw();
			auto& rq = ctx.Request();
			rq.text = Title;
			rq.canvasMode = Text::CanvasMode::Fixed;
			rq.clearMode = Text::ClearMode::NoClear;
			rq.primaryFont = Text::FontSource::File(L"asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf");
			rq.style.fontSize = 40.0f;
			rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
			rq.style.wordWrapEnabled = false;
			rq.spans = { Text::Span{.start = 0, .length = static_cast<UINT32>(Title.length()), .weight = DWRITE_FONT_WEIGHT_BOLD } };
			rq.maxWidthPx = titleWidth;
			rq.paddingPx = titleHeight;
			rq.defaultColor = Colors::White;
			rq.backgroundColor = Colors::None;
			ctx.Render(*titleCanvas);
		}
		// Buttons
		{
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

			uiRoot = std::make_unique<Ui::UiRoot>();
			uiRoot->Clear();
			btnA_->RegisterTo(*uiRoot);
			btnB_->RegisterTo(*uiRoot);
			btnC_->RegisterTo(*uiRoot);
			uiRoot->RebuildTabOrderFromSlots();
			uiRoot->InitLinkTechniques(rg);
		}
	}
	~UI_Setting() = default;

	void Update(float dt)
	{
		uiRoot->UpdateAfterInput();
	}

	void Submit(void)
	{
		uiRoot->Submit(Chan::ui);

		titleCanvas->Submit(Chan::ui);
	}

	void SetOnNewGame(std::function<void()> cb) { onNewGame_ = std::move(cb); }

private:
	// Title
	std::unique_ptr<Canvas2D> titleCanvas;

	// Buttons
	std::unique_ptr<Ui::UiRoot> uiRoot;
	std::unique_ptr<Ui::ButtonCanvasComponent> btnA_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnC_{};

	std::function<void()> onNewGame_{};
};