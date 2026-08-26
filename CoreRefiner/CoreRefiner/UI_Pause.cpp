#include "UI_Pause.h"

#include "Channels.h"
#include "Colors.h"
#include "RenderGraph.h"
#include "TextTypes.h"

namespace
{
	constexpr Color kDimColor{ 32u, 32u, 36u, 160u };
	constexpr Ui::FocusHandle kContinueFocus{ 601u };
	constexpr Ui::FocusHandle kQuitFocus{ 602u };
}

UI_Pause::UI_Pause(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned centerX = SCREEN_WIDTH / 2u;
	const unsigned centerY = SCREEN_HEIGHT / 2u;
	const unsigned spacingY = SCREEN_HEIGHT / 20u;
	const unsigned btnWidth = SCREEN_WIDTH / 10u;
	const unsigned btnHeight = SCREEN_HEIGHT / 15u;

	// Background
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

	Ui::ButtonCanvasStyle style{};
	style.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	uiRoot_ = std::make_unique<Ui::UiRoot>();
	uiRoot_->Clear();

	// Button Continue
	{
		btnContinue_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kContinueFocus,
			static_cast<float>(centerX),
			static_cast<float>(centerY),
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			style);

		btnContinue_->Button().SetLabel("继续");

		btnContinue_->Button().SetOnClick([this] {
			if (onContinue_)
			{
				onContinue_();
			}
			});

		btnContinue_->RegisterTo(*uiRoot_);
	}

	// Button Quit
	{
		btnQuit_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kQuitFocus,
			static_cast<float>(centerX),
			static_cast<float>(centerY + spacingY * 2u),
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			style);

		btnQuit_->Button().SetLabel("退出");

		btnQuit_->Button().SetOnClick([this] {
			if (onQuit_)
			{
				onQuit_();
			}
			});
		btnQuit_->RegisterTo(*uiRoot_);
	}

	uiRoot_->RebuildTabOrder();
	uiRoot_->InitLinkTechniques(rg);
}

void UI_Pause::SetOnContinue(std::function<void()> cb)
{
	onContinue_ = std::move(cb);
}

void UI_Pause::SetOnQuit(std::function<void()> cb)
{
	onQuit_ = std::move(cb);
}

void UI_Pause::Show() noexcept
{
	open_ = true;
}

void UI_Pause::Hide() noexcept
{
	open_ = false;
}

bool UI_Pause::IsOpen() const noexcept
{
	return open_;
}

void UI_Pause::Update(float dt)
{
	(void)dt;
	if (!open_ || uiRoot_ == nullptr)
	{
		return;
	}
	uiRoot_->UpdateAfterInput();
}

void UI_Pause::Submit()
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
