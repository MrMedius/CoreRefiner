#include "UI_CombatHud.h"

#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Colors.h"
#include "ModuleField.h"
#include "TextCodex.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	[[nodiscard]] int RemainSecKey_(float remainSec) noexcept
	{
		const float clamped = (remainSec > 0.0f) ? remainSec : 0.0f;
		return static_cast<int>(std::ceil(clamped - 1.0e-4f));
	}

	[[nodiscard]] int HpKey_(float ratio) noexcept
	{
		const float clamped = std::clamp(ratio, 0.0f, 1.0f);
		return static_cast<int>(std::lround(clamped * 100.0f));
	}
}

UI_CombatHud::UI_CombatHud(Graphics& gfx, Rgph::RenderGraph& rg, ModuleField* field)
	:
	gfx_(gfx),
	rg_(rg),
	field_(field)
{
	EnsureVisuals_();
	PaintWave_();
	PaintTimer_();
	PaintHpBar_();
	SyncTransforms_();
}

void UI_CombatHud::SetWave(int wave)
{
	if (wave < 0)
	{
		wave = 0;
	}
	if (wave_ == wave && paintedWave_ == wave)
	{
		return;
	}
	wave_ = wave;
	PaintWave_();
	SyncTransforms_();
}

void UI_CombatHud::SetRemain(float remainSec)
{
	remainSec_ = (remainSec > 0.0f) ? remainSec : 0.0f;
	const int key = RemainSecKey_(remainSec_);
	if (key == paintedRemainSec_)
	{
		return;
	}
	PaintTimer_();
	SyncTransforms_();
}

void UI_CombatHud::SetHpRatio(float ratio)
{
	hpRatio_ = std::clamp(ratio, 0.0f, 1.0f);
	const int key = HpKey_(hpRatio_);
	if (key == paintedHpKey_)
	{
		return;
	}
	PaintHpBar_();
}

void UI_CombatHud::SubmitField()
{
	if (field_ == nullptr)
	{
		return;
	}
	field_->SubmitBackground();
	field_->SubmitNodes();
}

void UI_CombatHud::SubmitHud()
{
	if (waveText_ != nullptr)
	{
		waveText_->Submit(Chan::ui);
	}
	if (timerText_ != nullptr)
	{
		timerText_->Submit(Chan::ui);
	}
	if (hpBar_ != nullptr)
	{
		hpBar_->Submit(Chan::ui);
	}
}

void UI_CombatHud::Submit()
{
	SubmitField();
	SubmitHud();
}

void UI_CombatHud::EnsureVisuals_()
{
	if (waveText_ == nullptr)
	{
		waveText_ = std::make_unique<Canvas2D>(gfx_, 160u, 32u);
		waveText_->Clear(Colors::None);
		waveText_->LinkTechniques(rg_);
	}
	if (timerText_ == nullptr)
	{
		timerText_ = std::make_unique<Canvas2D>(gfx_, 96u, 36u);
		timerText_->Clear(Colors::None);
		timerText_->LinkTechniques(rg_);
	}
	if (hpBar_ == nullptr)
	{
		hpBar_ = std::make_unique<Canvas2D>(gfx_, kHpBarW_, kHpBarH_);
		hpBar_->Clear(Colors::None);
		hpBar_->LinkTechniques(rg_);
		hpBar_->SetScale(DirectX::XMFLOAT3{ static_cast<float>(kHpBarW_),static_cast<float>(kHpBarH_),1.0f });
	}
}

void UI_CombatHud::PaintWave_()
{
	EnsureVisuals_();
	if (waveText_ == nullptr)
	{
		return;
	}
	PaintText_(
		*waveText_,
		"WAVE " + std::to_string(wave_),
		kWaveFontSize_,
		paintedWave_,
		wave_);
}

void UI_CombatHud::PaintTimer_()
{
	EnsureVisuals_();
	if (timerText_ == nullptr)
	{
		return;
	}
	const int sec = RemainSecKey_(remainSec_);
	PaintText_(
		*timerText_,
		std::to_string(sec),
		kTimerFontSize_,
		paintedRemainSec_,
		sec);
}

void UI_CombatHud::PaintHpBar_()
{
	EnsureVisuals_();
	if (hpBar_ == nullptr)
	{
		return;
	}

	const unsigned lastX = kHpBarW_ - 1u;
	const unsigned lastY = kHpBarH_ - 1u;
	hpBar_->Clear(Color{ 24u, 26u, 32u, 220u });
	CanvasPixelDraw::DrawRectBorder(*hpBar_, 0u, 0u, lastX, lastY, 1u, Colors::White);

	const int key = HpKey_(hpRatio_);
	if (key > 0 && kHpBarW_ > 4u && kHpBarH_ > 4u)
	{
		const unsigned innerW = kHpBarW_ - 4u;
		const unsigned fillW = static_cast<unsigned>(
			std::max(1, (key * static_cast<int>(innerW) + 50) / 100));
		const unsigned x1 = 2u + fillW - 1u;
		CanvasPixelDraw::FillRect(*hpBar_, 2u, 2u, x1, lastY - 2u, Colors::Kita);
	}
	hpBar_->NotifyPixelsChanged();
	paintedHpKey_ = key;
}

void UI_CombatHud::PaintText_(Canvas2D& canvas, const std::string& text, float fontSize, int& paintedKey, int key)
{
	if (paintedKey == key)
	{
		return;
	}

	auto ctx = TextCodex::Get().BeginDraw();
	Text::RenderRequest& rq = ctx.Request();
	rq.text = text;
	rq.canvasMode = Text::CanvasMode::Auto;
	rq.clearMode = Text::ClearMode::Clear;
	rq.primaryFont = Text::FontSource::System(L"Microsoft YaHei UI");
	rq.fallbackFonts.clear();
	rq.fallbackFonts.push_back(Text::FontSource::System(L"Segoe UI"));
	rq.style.fontSize = fontSize;
	rq.style.wordWrapEnabled = false;
	rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	rq.maxWidthPx = 240.0f;
	rq.paddingPx = 4;
	rq.defaultColor = Colors::White;
	rq.backgroundColor = Colors::None;
	ctx.Render(canvas);

	const unsigned w = (std::max)(1u, canvas.GetCanvasWidth());
	const unsigned h = (std::max)(1u, canvas.GetCanvasHeight());
	canvas.SetScale(DirectX::XMFLOAT3{
		static_cast<float>(w),
		static_cast<float>(h),
		1.0f
	});
	paintedKey = key;
}

void UI_CombatHud::SyncTransforms_() noexcept
{
	const float cx = static_cast<float>(SCREEN_WIDTH) * 0.5f;
	const float waveY = 36.0f;
	const float timerY = 72.0f;
	const float hpX = 16.0f + static_cast<float>(kHpBarW_) * 0.5f;
	const float hpY = 16.0f + static_cast<float>(kHpBarH_) * 0.5f;

	if (waveText_ != nullptr)
	{
		waveText_->SetPosition(DirectX::XMFLOAT3{ cx, waveY, 0.0f });
	}
	if (timerText_ != nullptr)
	{
		timerText_->SetPosition(DirectX::XMFLOAT3{ cx, timerY, 0.0f });
	}
	if (hpBar_ != nullptr)
	{
		hpBar_->SetPosition(DirectX::XMFLOAT3{ hpX, hpY, 0.0f });
	}
}
