#pragma once
#include "Graphics.h"

#include "ButtonCanvasComponent.h"
#include "SliderCanvasComponent.h"
#include "StepperCanvasComponent.h"
#include "ToggleCanvasComponent.h"
#include "DropdownCanvasComponent.h"
#include "TextFieldCanvasComponent.h"
#include "UiOptionList.h"
#include "Canvas2DRipples.h"

#include "UiRoot.h"
#include "Canvas2D.h"
#include "TextCodex.h"

#include "FocusTypes.h"
#include "Channels.h"

#include <memory>
#include <functional>

#include "SoundCodex.h"

class UI_Sample
{
public:
	UI_Sample(Graphics& gfx, Rgph::RenderGraph& rg)
	{
		unsigned centerX = SCREEN_WIDTH / 2u;
		unsigned centerY = SCREEN_HEIGHT / 2u;

		unsigned titleWidth = SCREEN_WIDTH / 2u;
		unsigned titleHeight = SCREEN_HEIGHT / 5u;

		{
			titleBg = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleBg->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX) * 1.5f, static_cast<float>(centerY / 2u + titleHeight * 3), 0.0f });
			titleBg->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth / 2.0f), static_cast<float>(titleHeight / 2.0f), 1.0f });
			titleBg->LinkTechniques(rg);
			titleBg->GetParams().ringColor = (Colors::Kita - Color(0u, 0u, 0u, 50u)).ToFloat4();

			titleBg_2 = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleBg_2->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX) * 1.5f, static_cast<float>(centerY / 2u + titleHeight * 2), 0.0f });
			titleBg_2->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth / 2.0f), static_cast<float>(titleHeight / 2.0f), 1.0f });
			titleBg_2->LinkTechniques(rg);
			titleBg_2->GetParams().ringColor = (Colors::Kita - Color(0u, 0u, 0u, 50u)).ToFloat4();
		}
		{
			titleCanvas = std::make_unique<Canvas2D>(gfx, titleWidth, titleHeight);
			titleCanvas->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX) * 1.5f, static_cast<float>(centerY / 2u + titleHeight * 3), 0.0f });
			titleCanvas->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth / 2.0f), static_cast<float>(titleHeight / 2.0f), 1.0f });
			titleCanvas->LinkTechniques(rg);

			titleCanvas_2 = std::make_unique<Canvas2D>(gfx, titleWidth, titleHeight);
			titleCanvas_2->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX) * 1.5f, static_cast<float>(centerY / 2u), 0.0f });
			titleCanvas_2->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth / 2.0f), static_cast<float>(titleHeight / 2.0f), 1.0f });
			titleCanvas_2->LinkTechniques(rg);

			titleCanvasRipple = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleCanvasRipple->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX) * 1.5f, static_cast<float>(centerY / 2u + titleHeight * 3), 0.0f });
			titleCanvasRipple->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth / 2.0f), static_cast<float>(titleHeight / 2.0f), 1.0f });
			titleCanvasRipple->LinkTechniques(rg);

			titleCanvasRipple_2 = std::make_unique<Canvas2DRipples>(gfx, titleWidth, titleHeight);
			titleCanvasRipple_2->SetPosition(DirectX::XMFLOAT3{ static_cast<float>(centerX) * 1.5f, static_cast<float>(centerY / 2u + titleHeight), 0.0f });
			titleCanvasRipple_2->SetScale(DirectX::XMFLOAT3{ static_cast<float>(titleWidth / 2.0f), static_cast<float>(titleHeight / 2.0f), 1.0f });
			titleCanvasRipple_2->LinkTechniques(rg);

			std::string Title = "Testing テスト 测试 測試";
			auto ctx = TextCodex::Get().BeginDraw();
			auto& rq = ctx.Request();
			rq.text = Title;
			rq.canvasMode = Text::CanvasMode::Fixed;
			rq.clearMode = Text::ClearMode::Clear;
			rq.primaryFont = Text::FontSource::File(L"asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf");
			rq.style.fontSize = 50.0f;
			rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
			rq.style.wordWrapEnabled = true;
			rq.maxWidthPx = static_cast<float>(titleWidth);
			rq.defaultColor = Colors::White;
			rq.backgroundColor = Colors::None;
			ctx.Render(*titleCanvas);
			ctx.Render(*titleCanvas_2);
			ctx.Render(*titleCanvasRipple);
			ctx.Render(*titleCanvasRipple_2);

			for (unsigned y = 0; y < titleHeight; ++y)
			{
				for (unsigned x = 0; x < titleWidth; ++x)
				{
					if (y == 1 || y == titleHeight - 1 || x == 1 || x == titleWidth - 1)
					{
						titleCanvas->PutPixel(x, y, Colors::White);
						titleCanvas_2->PutPixel(x, y, Colors::White);
						titleCanvasRipple->PutPixel(x, y, Colors::White);
						titleCanvasRipple_2->PutPixel(x, y, Colors::White);
					}
				}
			}
		}
		{
			unsigned spacingY = SCREEN_HEIGHT / 20u;
			unsigned btnWidth = SCREEN_WIDTH / 10u;
			unsigned btnHeight = SCREEN_HEIGHT / 15u;

			// Button
			{
				btnA_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 501u, centerX - titleWidth / 1.5, centerY - spacingY * 6u, btnWidth, btnHeight);
				btnB_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 502u, centerX - titleWidth / 1.5, centerY - spacingY * 4u, btnWidth, btnHeight);
				btnC_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, 503u, centerX - titleWidth / 1.5, centerY - spacingY * 2u, btnWidth, btnHeight);

				btnA_->Button().SetLabel("Mute");
				btnB_->Button().SetLabel("TestBtn");
				btnC_->Button().SetLabel("Quit");

				btnA_->Button().SetOnClick([this] { hSlider_->Slider().SetValue(0.0f); });
				btnB_->Button().SetOnClick([this] { btnB_->Button().SetLabel("TestBtn"); });
				btnC_->Button().SetOnClick([this] { PostQuitMessage(0); });
			}

			// Slider
			{
				const float hBarW = static_cast<float>(titleWidth) * 0.8f;
				const float hBarH = 24.0f;
				const float hBarY = static_cast<float>(centerY);

				hSlider_2 = std::make_unique<Ui::SliderCanvasComponent>(
					gfx, 504u,
					static_cast<float>(centerX) - titleWidth / 3 - hBarH, hBarY, hBarW, hBarH,
					true);
				hSlider_2->Slider().SetValue(0.45f);
				hSlider_2->Slider().SetInteractive(true);

				hSlider_ = std::make_unique<Ui::SliderCanvasComponent>(
					gfx, 0u,
					static_cast<float>(centerX) - titleWidth / 3, hBarY + spacingY * 1.5, hBarW, hBarH,
					true);
				hSlider_->Slider().SetValue(0.2f);
				hSlider_->Slider().SetInteractive(false);
				hSlider_->Slider().SetOnValueChanged([this](float norm) {
					stepperSlider_->Stepper().SetValue(norm * 10.0f, false);
					});
				const float stepperSliderTotalW = hBarH * 2.0f + hBarW;
				stepperSlider_ = std::make_unique<Ui::StepperCanvasComponent>(
					gfx, 505u,
					static_cast<float>(centerX) - titleWidth / 3, hBarY + spacingY * 1.5, stepperSliderTotalW, hBarH);
				stepperSlider_->SetCenterVisible(false);
				stepperSlider_->SetCenterGapWidth(hBarW);
				stepperSlider_->Stepper().SetRange(0.0f, 10.0f);
				stepperSlider_->Stepper().SetValue(3.5f);
				stepperSlider_->Stepper().SetOnValueChanged([this](float v) {
					hSlider_->Slider().SetValue(v / 10.0f);
					});

				const float vBarW = 24.0f;
				const float vBarH = static_cast<float>(titleHeight);
				const float vBarX = static_cast<float>(centerX) - static_cast<float>(titleWidth / 3);
				const float vBarY = static_cast<float>(centerY / 2);
				vSlider_ = std::make_unique<Ui::SliderCanvasComponent>(
					gfx, 506u,
					vBarX, vBarY, vBarH, vBarW,
					true, Ui::SliderCanvasStyle{}, 45.0f);
				vSlider_->Slider().SetValue(0.6f);
			}

			// Toggle
			{
				const float toggleSize = static_cast<float>(btnHeight);
				const float toggleY = static_cast<float>(centerY) - spacingY * 6.5f;
				toggleSound_ = std::make_unique<Ui::ToggleCanvasComponent>(
					gfx, 511u, 600.0f, toggleY, toggleSize);
				toggleSound_->Toggle().SetIsOn(true, false);
				toggleSound_->SetLayoutBoxScale(0.6f);

				toggleFullscreen_ = std::make_unique<Ui::ToggleCanvasComponent>(
					gfx, 512u, 600.0f, toggleY + static_cast<float>(spacingY) * 2.0f, toggleSize * 1.25f);
			}

			// Stepper
			{
				const float stepperDemoH = static_cast<float>(btnHeight);
				const float stepperDemoTextW = static_cast<float>(btnWidth);
				const float stepperDemoTotalW = stepperDemoH * 2.0f + stepperDemoTextW;
				const float stepperDemoX = static_cast<float>(centerX) - static_cast<float>(btnWidth) * 3.0f;
				const float stepperDemoY = static_cast<float>(centerY) + static_cast<float>(spacingY) * 3.5f;
				stepperDemo_ = std::make_unique<Ui::StepperCanvasComponent>(
					gfx, 516u, stepperDemoX, stepperDemoY, stepperDemoTotalW, stepperDemoH);
				stepperDemo_->Stepper().SetLabel("Limit: 10");
				stepperDemo_->Stepper().SetRange(0.0f, 30.0f);
				stepperDemo_->Stepper().SetValue(10.0f);
				stepperDemo_->Stepper().SetOnValueChanged([this](float v) {
					stepperDemo_->Stepper().SetLabel(
						"Limit: " + std::to_string(static_cast<int>(v)));
					if (textField_)
						textField_->Field().SetMaxLength(v);
					});

				qualityOptions_ = std::make_unique<Ui::UiOptionList>();
				qualityOptions_->SetOptions({
					{.label = "Low" },
					{.label = "Mid" },
					{.label = "High" },
					{.label = "Utral" },
					});
				qualityOptions_->SetOnSelectionChanged([this](int, const std::string& label) {
					btnB_->Button().SetLabel("Step: " + label);
					stepperQuality_->Stepper().SetLabel(label);
					stepperQuality_->Stepper().SetValue(
						static_cast<float>(qualityOptions_->GetSelectedIndex()), false);
					});

				const float qualityY = static_cast<float>(centerY) - static_cast<float>(spacingY) * 2.0f;
				const float qualityX = static_cast<float>(centerX) - 150.0f;
				const float stepperQualityTextW = static_cast<float>(btnWidth) * 1.5f;
				const float stepperQualityTotalW = static_cast<float>(btnHeight) * 2.0f + stepperQualityTextW;
				stepperQuality_ = std::make_unique<Ui::StepperCanvasComponent>(
					gfx, 517u, qualityX, qualityY, stepperQualityTotalW, static_cast<float>(btnHeight));
				stepperQuality_->Stepper().SetLabel(qualityOptions_->GetSelectedLabel());
				stepperQuality_->Stepper().SetRange(
					0.0f, static_cast<float>(qualityOptions_->Count() - 1));
				stepperQuality_->Stepper().SetStep(1.0f);
				stepperQuality_->Stepper().SetValue(
					static_cast<float>(qualityOptions_->GetSelectedIndex()));
				stepperQuality_->Stepper().SetOnValueChanged([this](float v) {
					qualityOptions_->SetSelectedIndex(static_cast<int>(v));
					});
			}

			// Dropdown
			{
				const float dropdownW = static_cast<float>(btnWidth) * 2.0f;
				const float dropdownH = static_cast<float>(btnHeight);
				const float dropdownX = static_cast<float>(centerX) - static_cast<float>(btnWidth) * 2.9f;
				const float dropdownY = static_cast<float>(centerY) + static_cast<float>(spacingY) * 8.5f;

				customOptions_ = std::make_unique<Ui::UiOptionList>();
				customOptions_->SetOptions({
					{.label = "Font Size 16" },
					{.label = "Font Size 18" },
					{.label = "Font Size 20" },
					{.label = "Font Size 22" },
					{.label = "Font Size 24" },
					{.label = "Font Size 26" },
					{.label = "Font Size 28" },
					{.label = "Font Size 30" },
					});
				customOptions_->AddOption({ .label = "Font Size 32" });

				dropdownCustom_ = std::make_unique<Ui::DropdownCanvasComponent>(
					gfx, 514u,
					dropdownX,
					dropdownY,
					dropdownW, dropdownH);
				dropdownCustom_->BindOptionList(*customOptions_);
				customOptions_->SetOnSelectionChanged([this](int, const std::string& label) {
					if (!textField_)
						return;
					const std::size_t pos = label.find_last_of(' ');
					if (pos == std::string::npos)
						return;
					try
					{
						const float fontSize = std::stof(label.substr(pos + 1));
						textField_->Field().SetFontSize(fontSize);
					}
					catch (...)
					{}
					});
			}

			// TextField
			{
				const float textFieldW = static_cast<float>(btnWidth) * 2.5f;
				const float textFieldH = static_cast<float>(btnHeight);
				const float textFieldX = static_cast<float>(centerX) - static_cast<float>(btnWidth) * 2.65f;
				const float textFieldY = static_cast<float>(centerY) + static_cast<float>(spacingY) * 6.0f;
				textField_ = std::make_unique<Ui::TextFieldCanvasComponent>(
					gfx, 518u, textFieldX, textFieldY, textFieldW, textFieldH);
				textField_->Field().SetPlaceholder("Enter name...");
				textField_->Field().SetMaxLength(10);
				textField_->Field().SetFontSize(16.0f);
				textField_->Field().SetOnTextChanged([this](const std::string& s) {
					btnB_->Button().SetLabel(s.empty() ? "TestBtn" : ("Name: " + s));
					});
			}


			uiRoot = std::make_unique<Ui::UiRoot>();
			uiRoot->Clear();
			btnA_->RegisterTo(*uiRoot);
			btnB_->RegisterTo(*uiRoot);
			btnC_->RegisterTo(*uiRoot);

			vSlider_->RegisterTo(*uiRoot);

			toggleSound_->RegisterTo(*uiRoot);
			toggleFullscreen_->RegisterTo(*uiRoot);

			stepperQuality_->RegisterTo(*uiRoot);

			hSlider_2->RegisterTo(*uiRoot);
			hSlider_->RegisterTo(*uiRoot);
			stepperSlider_->RegisterTo(*uiRoot);

			stepperDemo_->RegisterTo(*uiRoot);
			textField_->RegisterTo(*uiRoot);

			dropdownCustom_->RegisterTo(*uiRoot);

			uiRoot->RebuildTabOrder();
			uiRoot->InitLinkTechniques(rg);
		}
	}
	~UI_Sample() = default;

	void Update(float dt)
	{
		uiRoot->UpdateAfterInput();

		SoundCodex::Get().SetBgmVolume(hSlider_->Slider().GetValue());
	}

	void Submit(void)
	{
		titleBg->Submit(Chan::ui);
		titleBg_2->Submit(Chan::ui);

		titleCanvas->Submit(Chan::ui);
		titleCanvas_2->Submit(Chan::ui);

		titleCanvasRipple->Submit(Chan::ui);
		titleCanvasRipple_2->Submit(Chan::ui);

		uiRoot->Submit(Chan::ui);
	}

private:
	std::unique_ptr<Canvas2DRipples> titleBg;
	std::unique_ptr<Canvas2DRipples> titleBg_2;
	std::unique_ptr<Canvas2D> titleCanvas;
	std::unique_ptr<Canvas2D> titleCanvas_2;
	std::unique_ptr<Canvas2DRipples> titleCanvasRipple;
	std::unique_ptr<Canvas2DRipples> titleCanvasRipple_2;

	std::unique_ptr<Ui::UiRoot> uiRoot;

	std::unique_ptr<Ui::ButtonCanvasComponent> btnA_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnB_{};
	std::unique_ptr<Ui::ButtonCanvasComponent> btnC_{};

	std::unique_ptr<Ui::SliderCanvasComponent> hSlider_{};
	std::unique_ptr<Ui::StepperCanvasComponent> stepperSlider_{};
	std::unique_ptr<Ui::SliderCanvasComponent> hSlider_2{};
	std::unique_ptr<Ui::SliderCanvasComponent> vSlider_{};

	std::unique_ptr<Ui::ToggleCanvasComponent> toggleSound_{};
	std::unique_ptr<Ui::ToggleCanvasComponent> toggleFullscreen_{};

	std::unique_ptr<Ui::UiOptionList> customOptions_{};
	std::unique_ptr<Ui::DropdownCanvasComponent> dropdownCustom_{};

	std::unique_ptr<Ui::StepperCanvasComponent> stepperDemo_{};
	std::unique_ptr<Ui::UiOptionList> qualityOptions_{};
	std::unique_ptr<Ui::StepperCanvasComponent> stepperQuality_{};
	std::unique_ptr<Ui::TextFieldCanvasComponent> textField_{};

	std::function<void()> onNewGame_{};
};