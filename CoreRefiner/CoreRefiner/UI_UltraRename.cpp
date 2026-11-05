#include "UI_UltraRename.h"

#include "Channels.h"
#include "Colors.h"
#include "GameRulesCodex.h"
#include "RenderGraph.h"
#include "UiCopy.h"

namespace
{
	constexpr Color kDimColor{ 32u, 32u, 36u, 160u };
	constexpr Ui::FocusHandle kFieldFocus{ 821u };
	constexpr Ui::FocusHandle kConfirmFocus{ 822u };
	constexpr Ui::FocusHandle kExitFocus{ 823u };
	constexpr float kFieldFont = 28.0f;
}

UI_UltraRename::UI_UltraRename(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned centerX = SCREEN_WIDTH / 2u;
	const unsigned centerY = SCREEN_HEIGHT / 2u;
	const unsigned spacingY = SCREEN_HEIGHT / 20u;
	const unsigned fieldW = SCREEN_WIDTH / 3u;
	const unsigned fieldH = SCREEN_HEIGHT / 12u;
	const unsigned btnWidth = SCREEN_WIDTH / 8u;
	const unsigned btnHeight = SCREEN_HEIGHT / 15u;

	// 全屏半透明遮罩。
	{
		bg_ = std::make_unique<Canvas2D>(gfx, SCREEN_WIDTH, SCREEN_HEIGHT);
		bg_->Clear(kDimColor);
		bg_->SetPosition(DirectX::XMFLOAT3{
			static_cast<float>(centerX),
			static_cast<float>(centerY),
			0.0f
			});
		bg_->SetScale(DirectX::XMFLOAT3{
			static_cast<float>(SCREEN_WIDTH),
			static_cast<float>(SCREEN_HEIGHT),
			1.0f
			});
		bg_->LinkTechniques(rg);
	}

	Ui::ButtonCanvasStyle buttonStyle{};
	buttonStyle.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");

	Ui::TextFieldCanvasStyle fieldStyle{};
	fieldStyle.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	fieldStyle.fontSize = kFieldFont;

	uiRoot_ = std::make_unique<Ui::UiRoot>();
	uiRoot_->Clear();

	{
		field_ = std::make_unique<Ui::TextFieldCanvasComponent>(
			gfx, kFieldFocus,
			static_cast<float>(centerX),
			static_cast<float>(centerY - spacingY * 2u),
			static_cast<float>(fieldW),
			static_cast<float>(fieldH),
			fieldStyle);
		field_->Field().SetFontSize(kFieldFont);
		field_->Field().SetMaxLength(GameRulesCodex::UltraNameMaxCodepoints);
		field_->RegisterTo(*uiRoot_);
	}

	{
		btnConfirm_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kConfirmFocus,
			static_cast<float>(centerX),
			static_cast<float>(centerY),
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			buttonStyle);

		btnConfirm_->Button().SetOnClick([this] {
			if (!onConfirm_ || field_ == nullptr)
			{
				return;
			}
			onConfirm_(field_->Field().GetText());
			});

		btnConfirm_->RegisterTo(*uiRoot_);
	}

	{
		btnExit_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kExitFocus,
			static_cast<float>(centerX),
			static_cast<float>(centerY + spacingY * 2u),
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			buttonStyle);

		btnExit_->Button().SetOnClick([this] {
			if (onExit_)
			{
				onExit_();
			}
			});

		btnExit_->RegisterTo(*uiRoot_);
	}

	uiRoot_->RebuildTabOrder();
	uiRoot_->InitLinkTechniques(rg);
	RefreshLabels();
}

void UI_UltraRename::SetOnConfirm(std::function<void(std::string name)> cb)
{
	onConfirm_ = std::move(cb);
}

void UI_UltraRename::SetOnExit(std::function<void()> cb)
{
	onExit_ = std::move(cb);
}

void UI_UltraRename::Show(std::string currentName)
{
	open_ = true;
	if (field_ != nullptr)
	{
		field_->Field().SetText(std::move(currentName));
	}
	if (uiRoot_ != nullptr)
	{
		uiRoot_->Focus().RequestFocus(kFieldFocus);
	}
	RefreshLabels();
}

void UI_UltraRename::Hide() noexcept
{
	open_ = false;
}

bool UI_UltraRename::IsOpen() const noexcept
{
	return open_;
}

void UI_UltraRename::RefreshLabels()
{
	if (btnConfirm_ != nullptr)
	{
		btnConfirm_->Button().SetLabel(GetUiCopy("ui.confirm"));
	}
	if (btnExit_ != nullptr)
	{
		btnExit_->Button().SetLabel(GetUiCopy("ui.exit"));
	}
}

void UI_UltraRename::Update(float dt)
{
	(void)dt;
	if (!open_ || uiRoot_ == nullptr)
	{
		return;
	}
	uiRoot_->UpdateAfterInput();
}

void UI_UltraRename::Submit()
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
