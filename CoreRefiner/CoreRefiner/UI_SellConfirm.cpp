#include "UI_SellConfirm.h"

#include "CanvasTextDraw.h"
#include "Channels.h"
#include "Colors.h"
#include "RenderGraph.h"
#include "UiCopy.h"

namespace
{
	constexpr Color kDimColor{ 32u, 32u, 36u, 160u };
	constexpr Ui::FocusHandle kConfirmFocus{ 811u };
	constexpr Ui::FocusHandle kExitFocus{ 812u };
	constexpr unsigned kPromptW = 960u;
	constexpr unsigned kPromptH = 64u;
	constexpr float kPromptFont = 32.0f;
}

UI_SellConfirm::UI_SellConfirm(Graphics& gfx, Rgph::RenderGraph& rg)
{
	const unsigned centerX = SCREEN_WIDTH / 2u;
	const unsigned centerY = SCREEN_HEIGHT / 2u;
	const unsigned spacingY = SCREEN_HEIGHT / 20u;
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

	// 卖价提示。Show 时再写成当前卖价。
	{
		prompt_ = std::make_unique<Canvas2D>(gfx, kPromptW, kPromptH);
		prompt_->Clear(Colors::None);
		prompt_->SetPosition(DirectX::XMFLOAT3{
			static_cast<float>(centerX),
			static_cast<float>(centerY - spacingY * 2u),
			0.0f
			});
		prompt_->SetScale(DirectX::XMFLOAT3{
			static_cast<float>(kPromptW),
			static_cast<float>(kPromptH),
			1.0f
			});
		prompt_->LinkTechniques(rg);
	}

	Ui::ButtonCanvasStyle style{};
	style.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	uiRoot_ = std::make_unique<Ui::UiRoot>();
	uiRoot_->Clear();

	{
		btnConfirm_ = std::make_unique<Ui::ButtonCanvasComponent>(
			gfx, kConfirmFocus,
			static_cast<float>(centerX),
			static_cast<float>(centerY),
			static_cast<float>(btnWidth),
			static_cast<float>(btnHeight),
			style);

		btnConfirm_->Button().SetOnClick([this] {
			if (onConfirm_)
			{
				onConfirm_();
			}
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
			style);

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

void UI_SellConfirm::SetOnConfirm(std::function<void()> cb)
{
	onConfirm_ = std::move(cb);
}

void UI_SellConfirm::SetOnExit(std::function<void()> cb)
{
	onExit_ = std::move(cb);
}

void UI_SellConfirm::Show(int price)
{
	price_ = price;
	open_ = true;
	RefreshLabels();
}

void UI_SellConfirm::Hide() noexcept
{
	open_ = false;
}

bool UI_SellConfirm::IsOpen() const noexcept
{
	return open_;
}

void UI_SellConfirm::RefreshLabels()
{
	PaintPrompt_();
	if (btnConfirm_ != nullptr)
	{
		btnConfirm_->Button().SetLabel(GetUiCopy("ui.confirm"));
	}
	if (btnExit_ != nullptr)
	{
		btnExit_->Button().SetLabel(GetUiCopy("ui.exit"));
	}
}

void UI_SellConfirm::Update(float dt)
{
	(void)dt;
	if (!open_ || uiRoot_ == nullptr)
	{
		return;
	}
	uiRoot_->UpdateAfterInput();
}

void UI_SellConfirm::Submit()
{
	if (!open_)
	{
		return;
	}
	if (bg_ != nullptr)
	{
		bg_->Submit(Chan::ui);
	}
	if (prompt_ != nullptr)
	{
		prompt_->Submit(Chan::ui);
	}
	if (uiRoot_ != nullptr)
	{
		uiRoot_->Submit(Chan::ui);
	}
}

void UI_SellConfirm::PaintPrompt_()
{
	if (prompt_ == nullptr)
	{
		return;
	}
	prompt_->Clear(Colors::None);
	CanvasTextDraw::Draw(
		*prompt_,
		GetUiCopy("shop.sell.confirm", price_),
		kPromptFont,
		0,
		0,
		static_cast<int>(prompt_->GetCanvasWidth()),
		static_cast<int>(prompt_->GetCanvasHeight()),
		Colors::White,
		false,
		{},
		0,
		DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	prompt_->NotifyPixelsChanged();
}
