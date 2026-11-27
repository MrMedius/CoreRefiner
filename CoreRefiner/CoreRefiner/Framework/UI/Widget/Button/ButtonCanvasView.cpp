#include "ButtonCanvasView.h"

#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "TextCodex.h"

namespace Ui
{
	namespace
	{
		[[nodiscard]] bool IsDisabledPhase(UiVisualPhase phase) noexcept
		{
			return phase == UiVisualPhase::Disabled;
		}
	}

	ButtonCanvasView::ButtonCanvasView(Graphics& gfx, unsigned pixelWidth, unsigned pixelHeight, ButtonCanvasStyle style)
		:
		style_(std::move(style)),
		bgCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight)),
		textCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight)),
		ringCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight))
	{
		ringCanvas_->Clear(Colors::None);
	}

	void ButtonCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		bgCanvas_->LinkTechniques(rg);
		textCanvas_->LinkTechniques(rg);
		ringCanvas_->LinkTechniques(rg);
	}

	void ButtonCanvasView::Submit(std::size_t channelMask) const
	{
		bgCanvas_->Submit(channelMask);
		textCanvas_->Submit(channelMask);
		ringCanvas_->Submit(channelMask);
	}

	Color ButtonCanvasView::BackgroundForPhase(const UiVisualPhase phase) const noexcept
	{
		switch (phase)
		{
		case UiVisualPhase::Disabled: return style_.bgDisabled;
		case UiVisualPhase::Pressed:  return style_.bgPressed;
		case UiVisualPhase::Focused:  return style_.bgFocused;
		default:                      return style_.bgNormal;
		}
	}

	Color ButtonCanvasView::TextColorForPhase(const UiVisualPhase phase) const noexcept
	{
		return IsDisabledPhase(phase) ? style_.textDisabled : style_.textNormal;
	}

	void ButtonCanvasView::ApplyLayout(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		const DirectX::XMFLOAT3 pos{ centerX, centerY, 0.0f };
		const DirectX::XMFLOAT3 scale{ width, height, 1.0f };
		bgCanvas_->SetPosition(pos);
		bgCanvas_->SetScale(scale);
		textCanvas_->SetPosition(pos);
		textCanvas_->SetScale(scale);
		ringCanvas_->SetPosition(pos);
		ringCanvas_->SetScale(scale);
	}

	void ButtonCanvasView::RepaintBackground_(const UiVisualPhase phase)
	{
		bgCanvas_->Clear(BackgroundForPhase(phase));
	}

	void ButtonCanvasView::RepaintText_(const ButtonViewModel& vm)
	{
		Canvas& c = *textCanvas_;
		c.Clear(Colors::None);

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = vm.labelUtf8;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = style_.fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
		rq.style.wordWrapEnabled = true;
		rq.paddingPx = style_.paddingPx;
		rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
		rq.defaultColor = TextColorForPhase(vm.phase);
		rq.backgroundColor = Colors::None;

		ctx.Render(c);
	}

	void ButtonCanvasView::RepaintFocusRing_(const UiVisualPhase phase)
	{
		Canvas& c = *ringCanvas_;
		c.Clear(Colors::None);
		if (phase == UiVisualPhase::Focused)
			CanvasPixelDraw::DrawFocusRing(c, style_.focusRingColor, style_.focusRingThicknessPx);
	}

	void ButtonCanvasView::SyncFrom(const ButtonViewModel& vm)
	{
		const bool bgDirty = !hasPainted_
			|| vm.phase != lastPainted_.phase;

		const bool textDirty = !hasPainted_
			|| vm.labelUtf8 != lastPainted_.labelUtf8
			|| IsDisabledPhase(vm.phase) != IsDisabledPhase(lastPainted_.phase);

		const bool ringDirty = !hasPainted_
			|| (vm.phase == UiVisualPhase::Focused) != (lastPainted_.phase == UiVisualPhase::Focused);

		if (bgDirty)
			RepaintBackground_(vm.phase);

		if (textDirty)
			RepaintText_(vm);

		if (ringDirty)
			RepaintFocusRing_(vm.phase);

		if (bgDirty || textDirty || ringDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
