#pragma once
#include "Graphics.h"
#include "ButtonCanvasComponent.h"
#include "ProgressBarCanvasComponent.h"
#include "Canvas2DRipples.h"

#include "UiRoot.h"
#include "Canvas2D.h"
#include "TextCodex.h"

#include "Channels.h"

#include <cmath>
#include <memory>
#include <functional>

class UI_Title
{
public:
	UI_Title(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		// Center of the screen
		unsigned centerX = SCREEN_WIDTH / 2u;
		unsigned centerY = SCREEN_HEIGHT / 2u;

		unsigned titleWidth = SCREEN_WIDTH / 2u;
		unsigned titleHeight = SCREEN_HEIGHT / 5u;

		// Title Background
		{
			titleBg = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleBg->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX), static_cast<float>(centerY / 2u), 0.0f });
			titleBg->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth), static_cast<float>(titleHeight), 1.0f });
			titleBg->LinkTechniques(rg);
			titleBg->GetParams().ringColor = (Colors::Kita - Color(0u, 0u, 0u, 50u)).ToFloat4();
		}
		// Title
		{
			titleCanvas = std::make_unique<Canvas2D>(gfx, titleWidth, titleHeight);
			titleCanvas->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX), static_cast<float>(centerY / 2u), 0.0f });
			titleCanvas->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth), static_cast<float>(titleHeight), 1.0f });
			titleCanvas->LinkTechniques(rg);

			titleCanvasRipple = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleCanvasRipple->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX), static_cast<float>(centerY / 2u), 0.0f });
			titleCanvasRipple->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth), static_cast<float>(titleHeight), 1.0f });
			titleCanvasRipple->LinkTechniques(rg);

			// Text
			std::string Title = "CORE REFINER";
			auto ctx = TextCodex::Get().BeginDraw();
			auto& rq = ctx.Request();
			rq.text = Title;
			rq.canvasMode = Text::CanvasMode::Fixed;
			rq.clearMode = Text::ClearMode::Clear;
			rq.primaryFont = Text::FontSource::File(L"asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf");
			rq.style.fontSize = 100.0f;
			rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
			rq.style.wordWrapEnabled = true;
			rq.maxWidthPx = static_cast<float>(titleWidth);
			rq.defaultColor = Colors::White;
			rq.backgroundColor = Colors::None;
			ctx.Render(*titleCanvas);
			ctx.Render(*titleCanvasRipple);

			// Frame
			for (unsigned y = 0; y < titleHeight; ++y)
			{
				for (unsigned x = 0; x < titleWidth; ++x)
				{
					if (y == 0 || y == titleHeight - 1 || x == 0 || x == titleWidth - 1)
					{
						titleCanvas->PutPixel(x, y, Colors::White);
						titleCanvasRipple->PutPixel(x, y, Colors::White);
					}
				}
			}

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

			const float barW = static_cast<float>(titleWidth) * 0.8f;
			const float barH = 24.0f;
			const float barY = static_cast<float>(centerY / 2u) + static_cast<float>(titleHeight) * 0.5f + 20.0f;
			loadingBar_ = std::make_unique<Ui::ProgressBarCanvasComponent>(
				gfx, static_cast<float>(centerX), barY, barW, barH);
			loadingBar_->ProgressBar().SetValue(0.35f);

			uiRoot = std::make_unique<Ui::UiRoot>();
			uiRoot->Clear();
			loadingBar_->RegisterTo(*uiRoot);
			btnA_->RegisterTo(*uiRoot);
			btnB_->RegisterTo(*uiRoot);
			btnC_->RegisterTo(*uiRoot);
			uiRoot->RebuildTabOrder();
			uiRoot->InitLinkTechniques(rg);
		}
	}
	~UI_Title() = default;

	void Update(float dt)
	{
		loadTimer_ += dt;
		if (loadingBar_)
			loadingBar_->ProgressBar().SetValue(std::fmod(loadTimer_ * 0.15f, 1.0f));

		uiRoot->UpdateAfterInput();
	}

	void Submit(void)
	{
		// Title Background
		titleBg->Submit(Chan::ui);
		// Title
		titleCanvas->Submit(Chan::ui);
		titleCanvasRipple->Submit(Chan::ui);
		// Buttons
		uiRoot->Submit(Chan::ui);
	}

	void SetOnNewGame(std::function<void()> cb) { onNewGame_ = std::move(cb); }

private:
	// Title_Bg
	std::unique_ptr<Canvas2DRipples> titleBg;
	// Title
	std::unique_ptr<Canvas2D> titleCanvas;
	std::unique_ptr<Canvas2DRipples> titleCanvasRipple;
	// Buttons
	std::unique_ptr<Ui::UiRoot> uiRoot;
	std::unique_ptr<Ui::ButtonCanvasComponent> btnA_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnC_{};
	std::unique_ptr<Ui::ProgressBarCanvasComponent> loadingBar_{};

	float loadTimer_ = 0.0f;

	std::function<void()> onNewGame_{};
};