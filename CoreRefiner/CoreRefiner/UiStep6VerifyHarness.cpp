#include "UiStep6VerifyHarness.h"

#include <string>

#include "ButtonViewModel.h"
#include "Channels.h"
#include "InputCodex.h"
#include "RenderGraph.h"

#include "Win.h"

namespace
{
constexpr Ui::FocusHandle kFocusBtnA = 501u;
constexpr Ui::FocusHandle kFocusBtnB = 502u;

constexpr unsigned kCanvasLogicalW = 380u;
constexpr unsigned kCanvasLogicalH = 100u;
}

UiStep6VerifyHarness::~UiStep6VerifyHarness()
{
	Shutdown();
}

void UiStep6VerifyHarness::Init(Graphics& gfx, Rgph::RenderGraph& uiRg)
{
	if (initialized_)
		return;

	focus_.ClearTabOrder();
	focus_.SetTabOrder({ kFocusBtnA, kFocusBtnB });

	btnA_ = std::make_unique<Ui::UiButton>(
		kFocusBtnA,
		Ui::UiRect{ 200.0f, 300.0f, 580.0f, 400.0f });
	btnB_ = std::make_unique<Ui::UiButton>(
		kFocusBtnB,
		Ui::UiRect{ 700.0f, 300.0f, 1080.0f, 400.0f });

	btnA_->SetLabel("Btn A | clicks 0");
	btnB_->SetLabel("Btn B | clicks 0");

	btnA_->SetOnClick([this] {
		++clicksA_;
		btnA_->SetLabel("Btn A | clicks " + std::to_string(clicksA_));
	});
	btnB_->SetOnClick([this] {
		++clicksB_;
		btnB_->SetLabel("Btn B | clicks " + std::to_string(clicksB_));
	});

	viewA_ = std::make_unique<Ui::ButtonCanvasView>(gfx, kCanvasLogicalW, kCanvasLogicalH);
	viewB_ = std::make_unique<Ui::ButtonCanvasView>(gfx, kCanvasLogicalW, kCanvasLogicalH);

	// Quad center / scale align with logical hit boxes (logical canvas coords).
	viewA_->GetCanvas().SetPosition({ 390.0f, 350.0f, 0.0f });
	viewA_->GetCanvas().SetScale({ 380.0f, 100.0f, 1.0f });
	viewB_->GetCanvas().SetPosition({ 890.0f, 350.0f, 0.0f });
	viewB_->GetCanvas().SetScale({ 380.0f, 100.0f, 1.0f });

	viewA_->LinkTechniques(uiRg);
	viewB_->LinkTechniques(uiRg);

	auto va = Ui::MakeButtonViewModel(*btnA_);
	auto vb = Ui::MakeButtonViewModel(*btnB_);
	viewA_->SyncFrom(va);
	viewB_->SyncFrom(vb);

	initialized_ = true;
}

void UiStep6VerifyHarness::Shutdown()
{
	if (!initialized_)
		return;
	viewB_.reset();
	viewA_.reset();
	btnB_.reset();
	btnA_.reset();
	focus_.ClearTabOrder();
	clicksA_ = 0u;
	clicksB_ = 0u;
	initialized_ = false;
}

void UiStep6VerifyHarness::TickAfterInput(bool titleSceneActive)
{
	if (!initialized_ || !titleSceneActive)
		return;

	Ui::UiInputFrame frame = mouseAdapter_.BuildFrame(true);

	InputCodex& in = InputCodex::Get();
	if (in.KeyTriggered(VK_TAB))
	{
		if (in.KeyPressed(VK_SHIFT))
			frame.navigation.tabPrev = true;
		else
			frame.navigation.tabNext = true;
	}
	if (in.KeyTriggered(VK_RETURN) || in.KeyTriggered(VK_SPACE))
		frame.action.confirmPressed = true;

	focus_.ApplyNavigation(frame);

	btnA_->Update(frame, focus_);
	btnB_->Update(frame, focus_);

	viewA_->SyncFrom(Ui::MakeButtonViewModel(*btnA_));
	viewB_->SyncFrom(Ui::MakeButtonViewModel(*btnB_));
}

void UiStep6VerifyHarness::SubmitTitleUi(bool titleSceneActive)
{
	if (!initialized_ || !titleSceneActive)
		return;
	viewA_->Submit(Chan::ui);
	viewB_->Submit(Chan::ui);
}
