#pragma once
#include "Graphics.h"

#include "ButtonCanvasComponent.h"
#include "Canvas2DRipples.h"

#include "UiRoot.h"
#include "Canvas2D.h"
#include "TextCodex.h"
#include "UiCopy.h"

#include "Channels.h"

#include <memory>
#include <functional>

class UI_Title
{
public:
	UI_Title(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		unsigned centerX = SCREEN_WIDTH / 2u;
		unsigned centerY = SCREEN_HEIGHT / 2u;

		unsigned titleWidth = SCREEN_WIDTH / 2u;
		unsigned titleHeight = SCREEN_HEIGHT / 5u;

		{
			titleBg = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleBg->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX), static_cast<float>(centerY / 2u), 0.0f });
			titleBg->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth), static_cast<float>(titleHeight), 1.0f });
			titleBg->LinkTechniques(rg);
			titleBg->GetParams().ringColor = (Colors::Kita - Color(0u, 0u, 0u, 50u)).ToFloat4();
		}
		{
			titleCanvas = std::make_unique<Canvas2D>(gfx, titleWidth, titleHeight);
			titleCanvas->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX), static_cast<float>(centerY / 2u), 0.0f });
			titleCanvas->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth), static_cast<float>(titleHeight), 1.0f });
			titleCanvas->LinkTechniques(rg);

			titleCanvasRipple = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleCanvasRipple->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX), static_cast<float>(centerY / 2u), 0.0f });
			titleCanvasRipple->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth), static_cast<float>(titleHeight), 1.0f });
			titleCanvasRipple->LinkTechniques(rg);

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
		{
			unsigned spacingY = SCREEN_HEIGHT / 20u;
			unsigned btnWidth = SCREEN_WIDTH / 8u;
			unsigned btnHeight = SCREEN_HEIGHT / 15u;

			Ui::ButtonCanvasStyle style{};
			style.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");

			btnA_ = std::make_unique<Ui::ButtonCanvasComponent>(
				gfx, 501u,
				static_cast<float>(centerX),
				static_cast<float>(centerY + spacingY * 2u),
				static_cast<float>(btnWidth),
				static_cast<float>(btnHeight),
				style);
			btnB_ = std::make_unique<Ui::ButtonCanvasComponent>(
				gfx, 502u,
				static_cast<float>(centerX),
				static_cast<float>(centerY + spacingY * 4u),
				static_cast<float>(btnWidth),
				static_cast<float>(btnHeight),
				style);
			btnC_ = std::make_unique<Ui::ButtonCanvasComponent>(
				gfx, 503u,
				static_cast<float>(centerX),
				static_cast<float>(centerY + spacingY * 6u),
				static_cast<float>(btnWidth),
				static_cast<float>(btnHeight),
				style);

			btnA_->Button().SetOnClick([this] { if (onNewGame_) onNewGame_(); });
			btnB_->Button().SetOnClick([this] { if (onSettings_) onSettings_(); });
			btnC_->Button().SetOnClick([this] { PostQuitMessage(0); });

			RefreshLabels();

			uiRoot = std::make_unique<Ui::UiRoot>();
			uiRoot->Clear();
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
		(void)dt;
		uiRoot->UpdateAfterInput();
	}

	void Submit(void)
	{
		titleBg->Submit(Chan::ui);
		titleCanvas->Submit(Chan::ui);
		titleCanvasRipple->Submit(Chan::ui);
		uiRoot->Submit(Chan::ui);
	}

	// 按当前语言刷 New Game / Settings / Quit。
	void RefreshLabels()
	{
		if (btnA_ != nullptr)
		{
			btnA_->Button().SetLabel(GetUiCopy("title.new_game"));
		}
		if (btnB_ != nullptr)
		{
			btnB_->Button().SetLabel(GetUiCopy("title.settings"));
		}
		if (btnC_ != nullptr)
		{
			btnC_->Button().SetLabel(GetUiCopy("title.quit"));
		}
	}

	void SetOnNewGame(std::function<void()> cb) { onNewGame_ = std::move(cb); }
	void SetOnSettings(std::function<void()> cb) { onSettings_ = std::move(cb); }

private:
	std::unique_ptr<Canvas2DRipples> titleBg;
	std::unique_ptr<Canvas2D> titleCanvas;
	std::unique_ptr<Canvas2DRipples> titleCanvasRipple;

	std::unique_ptr<Ui::UiRoot> uiRoot;

	std::unique_ptr<Ui::ButtonCanvasComponent> btnA_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnC_{};

	std::function<void()> onNewGame_{};
	std::function<void()> onSettings_{};
};