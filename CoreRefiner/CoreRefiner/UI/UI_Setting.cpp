#include "UI_Setting.h"

#include "Channels.h"
#include "Colors.h"
#include "GameStatsCodex.h"
#include "JsonTextCopy.h"
#include "RenderGraph.h"
#include "SoundCodex.h"
#include "TextTypes.h"
#include "UiCopy.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	constexpr Color kPageColor{ 24u, 24u, 28u, 255u };

	constexpr Ui::FocusHandle kTitleFocus{ 720u };
	constexpr Ui::FocusHandle kLangFocus{ 701u };
	constexpr Ui::FocusHandle kMasterFocus{ 702u };
	constexpr Ui::FocusHandle kBgmFocus{ 703u };
	constexpr Ui::FocusHandle kSeFocus{ 704u };
	constexpr Ui::FocusHandle kMuteFocus{ 705u };
	constexpr Ui::FocusHandle kFullscreenFocus{ 706u };
	constexpr Ui::FocusHandle kWindowFocus{ 707u };
	constexpr Ui::FocusHandle kBackFocus{ 708u };
	constexpr Ui::FocusHandle kRowLabelFocus0{ 721u };

	constexpr const char* kRowCopyKeys[] = {
		"setting.language",
		"setting.master",
		"setting.bgm",
		"setting.se",
		"setting.mute",
		"setting.fullscreen",
		"setting.window",
	};

	constexpr const char* kLanguageNames[] = { "中文", "日本語", "English" };
	constexpr const char* kWindowSizeNames[] = { "1280x720", "1600x900", "1920x1080" };

	[[nodiscard]] Ui::ButtonCanvasStyle MakeChromeStyle_()
	{
		Ui::ButtonCanvasStyle style{};
		style.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		return style;
	}

	[[nodiscard]] Ui::ButtonCanvasStyle MakeLabelStyle_()
	{
		Ui::ButtonCanvasStyle style = MakeChromeStyle_();
		style.bgNormal = Colors::None;
		style.bgFocused = Colors::None;
		style.bgPressed = Colors::None;
		style.bgDisabled = Colors::None;
		style.textDisabled = Colors::White;
		style.fontSize = 18.0f;
		return style;
	}

	[[nodiscard]] Ui::ButtonCanvasStyle MakeTitleStyle_()
	{
		Ui::ButtonCanvasStyle style = MakeLabelStyle_();
		style.fontSize = 28.0f;
		return style;
	}

	[[nodiscard]] Ui::StepperCanvasStyle MakeStepperStyle_()
	{
		Ui::StepperCanvasStyle style{};
		style.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
		return style;
	}

	[[nodiscard]] int ClampIndex_(float value, int maxInclusive) noexcept
	{
		const int idx = static_cast<int>(std::lround(value));
		return std::clamp(idx, 0, maxInclusive);
	}
}

UI_Setting::UI_Setting(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const float centerX = static_cast<float>(SCREEN_WIDTH) * 0.5f;
	const float centerY = static_cast<float>(SCREEN_HEIGHT) * 0.5f;
	const float labelX = centerX - 240.0f;
	const float controlX = centerX + 140.0f;
	const float row0Y = 150.0f;
	const float rowStep = 58.0f;
	const float labelW = 220.0f;
	const float rowH = 48.0f;
	const float sliderW = 280.0f;
	const float sliderH = 28.0f;
	const float stepperW = 280.0f;

	// Background
	{
		bg_ = std::make_unique<Canvas2D>(gfx, SCREEN_WIDTH, SCREEN_HEIGHT);
		bg_->Clear(kPageColor);
		bg_->SetPosition(DirectX::XMFLOAT3{ centerX, centerY, 0.0f });
		bg_->SetScale(DirectX::XMFLOAT3{ static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), 1.0f });
		bg_->LinkTechniques(rg);
	}

	uiRoot_ = std::make_unique<Ui::UiRoot>();
	uiRoot_->Clear();

	// Title
	{
		title_ = std::make_unique<Ui::ButtonCanvasComponent>(gfx, kTitleFocus, centerX, 72.0f, 360.0f, 56.0f, MakeTitleStyle_());
		title_->Button().SetEnabled(false);
		title_->RegisterTo(*uiRoot_);
	}

	// Lable
	{
		const Ui::ButtonCanvasStyle labelStyle = MakeLabelStyle_();
		for (std::size_t i = 0; i < rowLabels_.size(); ++i)
		{
			const float y = row0Y + static_cast<float>(i) * rowStep;
			rowLabels_[i] = std::make_unique<Ui::ButtonCanvasComponent>(
				gfx,
				kRowLabelFocus0 + static_cast<Ui::FocusHandle>(i),
				labelX,
				y,
				labelW,
				rowH,
				labelStyle);
			rowLabels_[i]->Button().SetEnabled(false);
		}
		for (auto& label : rowLabels_)
		{
			label->RegisterTo(*uiRoot_);
		}
	}

	// Language
	{
		stepperLanguage_ = std::make_unique<Ui::StepperCanvasComponent>(gfx, kLangFocus, controlX, row0Y, stepperW, rowH, MakeStepperStyle_());
		stepperLanguage_->Stepper().SetRange(0.0f, static_cast<float>(LanguageCount() - 1u));
		stepperLanguage_->Stepper().SetStep(1.0f);
		stepperLanguage_->Stepper().SetOnValueChanged([this](float v) {
			const int idx = ClampIndex_(v, static_cast<int>(LanguageCount()) - 1);
			GameStatsCodex::SetLanguage(static_cast<Language>(idx));
			RefreshLanguageStepperLabel_();
			RefreshLabels();
			});
		stepperLanguage_->RegisterTo(*uiRoot_);
	}

	// Sound
	{
		sliderMaster_ = std::make_unique<Ui::SliderCanvasComponent>(gfx, kMasterFocus, controlX, row0Y + rowStep, sliderW, sliderH, true);
		sliderMaster_->Slider().SetOnValueChanged([this](float v) {
			GameStatsCodex::SetMasterVolume(v);
			ApplySoundToDevice_();
			});
		sliderMaster_->RegisterTo(*uiRoot_);

		sliderBgm_ = std::make_unique<Ui::SliderCanvasComponent>(gfx, kBgmFocus, controlX, row0Y + rowStep * 2.0f, sliderW, sliderH, true);
		sliderBgm_->Slider().SetOnValueChanged([this](float v) {
			GameStatsCodex::SetBgmVolume(v);
			ApplySoundToDevice_();
			});
		sliderBgm_->RegisterTo(*uiRoot_);
		
		sliderSe_ = std::make_unique<Ui::SliderCanvasComponent>(gfx, kSeFocus, controlX, row0Y + rowStep * 3.0f, sliderW, sliderH, true);
		sliderSe_->Slider().SetOnValueChanged([this](float v) {
			GameStatsCodex::SetSeVolume(v);
			ApplySoundToDevice_();
			});
		sliderSe_->RegisterTo(*uiRoot_);

		toggleMute_ = std::make_unique<Ui::ToggleCanvasComponent>(gfx, kMuteFocus, controlX, row0Y + rowStep * 4.0f, rowH);
		toggleMute_->SetLayoutBoxScale(0.6f);
		toggleMute_->Toggle().SetOnValueChanged([this](bool on) {
			GameStatsCodex::SetMuted(on);
			ApplySoundToDevice_();
			});
		toggleMute_->RegisterTo(*uiRoot_);
	}

	// Fullscreen
	{
		toggleFullscreen_ = std::make_unique<Ui::ToggleCanvasComponent>(gfx, kFullscreenFocus, controlX, row0Y + rowStep * 5.0f, rowH);
		toggleFullscreen_->SetLayoutBoxScale(0.6f);
		toggleFullscreen_->Toggle().SetOnValueChanged([this](bool on) {
			GameStatsCodex::SetFullscreen(on);
			if (onFullscreenChanged_)
			{
				onFullscreenChanged_(on);
			}
			});
		toggleFullscreen_->RegisterTo(*uiRoot_);
	}

	// Window
	{
		stepperWindow_ = std::make_unique<Ui::StepperCanvasComponent>(gfx, kWindowFocus, controlX, row0Y + rowStep * 6.0f, stepperW, rowH, MakeStepperStyle_());
		stepperWindow_->Stepper().SetRange(0.0f, static_cast<float>(GameStatsCodex::kWindowSizeCount - 1));
		stepperWindow_->Stepper().SetStep(1.0f);
		stepperWindow_->Stepper().SetOnValueChanged([this](float v) {
			const int idx = ClampIndex_(v, GameStatsCodex::kWindowSizeCount - 1);
			GameStatsCodex::SetWindowSizeIndex(idx);
			RefreshWindowStepperLabel_();
			if (onWindowSizeIndex_)
			{
				onWindowSizeIndex_(idx);
			}
			});
		stepperWindow_->RegisterTo(*uiRoot_);
	}

	// Back
	{
		btnBack_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kBackFocus, centerX, row0Y + rowStep * 7.5f, 160.0f, rowH, MakeChromeStyle_());
		btnBack_->Button().SetOnClick([this] {
			if (onBack_)
			{
				onBack_();
			}
			});
		btnBack_->RegisterTo(*uiRoot_);
	}

	SyncFromCodex_();
	RefreshLabels();

	uiRoot_->RebuildTabOrder();
	uiRoot_->InitLinkTechniques(rg);
}

void UI_Setting::SetOnBack(std::function<void()> cb)
{
	onBack_ = std::move(cb);
}

void UI_Setting::SetOnFullscreenChanged(std::function<void(bool)> cb)
{
	onFullscreenChanged_ = std::move(cb);
}

void UI_Setting::SetOnWindowSizeIndex(std::function<void(int)> cb)
{
	onWindowSizeIndex_ = std::move(cb);
}

void UI_Setting::Show()
{
	open_ = true;
	SyncFromCodex_();
	RefreshLabels();
}

void UI_Setting::Hide() noexcept
{
	open_ = false;
}

bool UI_Setting::IsOpen() const noexcept
{
	return open_;
}

void UI_Setting::RefreshLabels()
{
	if (title_ != nullptr)
	{
		title_->Button().SetLabel(GetUiCopy("title.settings"));
	}
	for (std::size_t i = 0; i < rowLabels_.size(); ++i)
	{
		if (rowLabels_[i] != nullptr)
		{
			rowLabels_[i]->Button().SetLabel(GetUiCopy(kRowCopyKeys[i]));
		}
	}
	if (btnBack_ != nullptr)
	{
		btnBack_->Button().SetLabel(GetUiCopy("setting.back"));
	}
}

void UI_Setting::Update(float dt)
{
	(void)dt;
	if (!open_ || uiRoot_ == nullptr)
	{
		return;
	}
	uiRoot_->UpdateAfterInput();
}

void UI_Setting::Submit()
{
	if (!open_)
	{
		return;
	}
	if (bg_ != nullptr)
	{
		bg_->Submit(Chan::ui);
	}
	if (uiRoot_ != nullptr)
	{
		uiRoot_->Submit(Chan::ui);
	}
}

void UI_Setting::SyncFromCodex_()
{
	const int langIdx = ClampIndex_(static_cast<float>(ToIndex(GameStatsCodex::GetLanguage())), static_cast<int>(LanguageCount()) - 1);
	stepperLanguage_->Stepper().SetValue(static_cast<float>(langIdx), false);
	RefreshLanguageStepperLabel_();

	sliderMaster_->Slider().SetValue(GameStatsCodex::GetMasterVolume());
	sliderBgm_->Slider().SetValue(GameStatsCodex::GetBgmVolume());
	sliderSe_->Slider().SetValue(GameStatsCodex::GetSeVolume());
	toggleMute_->Toggle().SetIsOn(GameStatsCodex::GetMuted(), false);

	toggleFullscreen_->Toggle().SetIsOn(GameStatsCodex::GetFullscreen(), false);

	const int windowIdx = ClampIndex_(static_cast<float>(GameStatsCodex::GetWindowSizeIndex()), GameStatsCodex::kWindowSizeCount - 1);
	stepperWindow_->Stepper().SetValue(static_cast<float>(windowIdx), false);

	RefreshWindowStepperLabel_();
	ApplySoundToDevice_();
}

void UI_Setting::ApplySoundToDevice_() const
{
	SoundCodex::Get().SetBgmVolume(GameStatsCodex::GetBgmVolume());
	SoundCodex::Get().SetSeVolume(GameStatsCodex::GetSeVolume());
	SoundCodex::Get().SetMasterVolume(GameStatsCodex::GetMuted() ? 0.0f : GameStatsCodex::GetMasterVolume());
}

void UI_Setting::RefreshLanguageStepperLabel_()
{
	const int idx = ClampIndex_(stepperLanguage_->Stepper().GetValue(), static_cast<int>(LanguageCount()) - 1);
	stepperLanguage_->Stepper().SetLabel(kLanguageNames[idx]);
}

void UI_Setting::RefreshWindowStepperLabel_()
{
	const int idx = ClampIndex_(stepperWindow_->Stepper().GetValue(), GameStatsCodex::kWindowSizeCount - 1);
	stepperWindow_->Stepper().SetLabel(kWindowSizeNames[idx]);
}
