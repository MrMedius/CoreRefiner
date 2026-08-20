#include "TextFieldCanvasView.h"

#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "TextCodex.h"

#include <algorithm>

namespace Ui
{
	namespace
	{
		[[nodiscard]] bool IsDisabledPhase(const UiVisualPhase phase) noexcept
		{
			return phase == UiVisualPhase::Disabled;
		}
	}

	TextFieldCanvasView::TextFieldCanvasView(
		Graphics& gfx,
		const unsigned pixelWidth,
		const unsigned pixelHeight,
		TextFieldCanvasStyle style)
		:
		style_(std::move(style)),
		bgCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight)),
		textCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight)),
		ringCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight)),
		caretCanvas_(std::make_unique<Canvas2D>(gfx, pixelWidth, pixelHeight))
	{
		ringCanvas_->Clear(Colors::None);
		caretCanvas_->Clear(Colors::None);
	}

	void TextFieldCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		bgCanvas_->LinkTechniques(rg);
		textCanvas_->LinkTechniques(rg);
		ringCanvas_->LinkTechniques(rg);
		caretCanvas_->LinkTechniques(rg);
	}

	void TextFieldCanvasView::Submit(const std::size_t channelMask) const
	{
		bgCanvas_->Submit(channelMask);
		textCanvas_->Submit(channelMask);
		caretCanvas_->Submit(channelMask);
		ringCanvas_->Submit(channelMask);
	}

	Color TextFieldCanvasView::BackgroundForPhase(const UiVisualPhase phase) const noexcept
	{
		switch (phase)
		{
		case UiVisualPhase::Disabled: return style_.bgDisabled;
		case UiVisualPhase::Focused:  return style_.bgFocused;
		default:                      return style_.bgNormal;
		}
	}

	Color TextFieldCanvasView::TextColorForPhase(const UiVisualPhase phase) const noexcept
	{
		return IsDisabledPhase(phase) ? style_.textDisabled : style_.textNormal;
	}

	void TextFieldCanvasView::ApplyLayout(
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
		caretCanvas_->SetPosition(pos);
		caretCanvas_->SetScale(scale);
	}

	std::string TextFieldCanvasView::BuildDisplayText_(const TextFieldViewModel& vm) const
	{
		if (vm.textUtf8.empty() && vm.imeCompositionActive)
			return vm.imeCompositionUtf8;

		if (!vm.imeCompositionActive || vm.imeCompositionUtf8.empty())
			return vm.textUtf8;

		std::string out = vm.textUtf8;
		const std::size_t index = std::min(vm.caretByteIndex, out.size());
		out.insert(index, vm.imeCompositionUtf8);
		return out;
	}

	std::size_t TextFieldCanvasView::CaretIndexInDisplay_(const TextFieldViewModel& vm) const
	{
		if (!vm.imeCompositionActive)
			return vm.caretByteIndex;

		return vm.caretByteIndex + vm.imeCompositionUtf8.size();
	}

	void TextFieldCanvasView::RepaintBackground_(const UiVisualPhase phase)
	{
		bgCanvas_->Clear(BackgroundForPhase(phase));
	}

	float TextFieldCanvasView::ComputeScrollOffsetX_(
		const std::string& displayText,
		const float fontSize) const
	{
		if (displayText.empty())
			return 0.0f;

		const int canvasW = static_cast<int>(textCanvas_->GetCanvasWidth());
		const int innerW = canvasW - style_.paddingPx * 2;
		if (innerW <= 0)
			return 0.0f;

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = displayText;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = style_.paddingPx;
		rq.maxWidthPx = static_cast<float>(canvasW);
		rq.backgroundColor = Colors::None;

		const Text::MeasureResult measure = ctx.Measure();
		const int textW = static_cast<int>(measure.widthPx) - style_.paddingPx * 2;
		if (textW <= innerW)
			return 0.0f;

		// 文字超出框宽时左移，使末端（输入处）保持可见
		return -static_cast<float>(textW - innerW);
	}

	void TextFieldCanvasView::RepaintText_(const TextFieldViewModel& vm, const float scrollOffsetX)
	{
		Canvas& c = *textCanvas_;
		c.Clear(Colors::None);

		const bool showPlaceholder = vm.textUtf8.empty()
			&& !vm.imeCompositionActive
			&& !vm.placeholderUtf8.empty();

		const std::string displayText = showPlaceholder
			? vm.placeholderUtf8
			: BuildDisplayText_(vm);

		if (displayText.empty())
			return;

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = displayText;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = style_.paddingPx;
		rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
		rq.drawOffsetXPx = showPlaceholder ? 0.0f : scrollOffsetX;
		rq.defaultColor = showPlaceholder ? style_.placeholderColor : TextColorForPhase(vm.phase);
		rq.backgroundColor = Colors::None;

		ctx.Render(c);
	}

	void TextFieldCanvasView::RepaintFocusRing_(const UiVisualPhase phase)
	{
		Canvas& c = *ringCanvas_;
		c.Clear(Colors::None);
		if (phase == UiVisualPhase::Focused)
			CanvasPixelDraw::DrawFocusRing(c, style_.focusRingColor, style_.focusRingThicknessPx);
	}

	void TextFieldCanvasView::RepaintCaret_(
		const TextFieldViewModel& vm,
		const std::string_view displayText,
		const std::size_t caretIndex,
		const float scrollOffsetX)
	{
		Canvas& c = *caretCanvas_;
		c.Clear(Colors::None);

		if (!vm.showCaret || vm.phase != UiVisualPhase::Focused)
			return;

		const std::string prefix(displayText.substr(0, std::min(caretIndex, displayText.size())));
		const float fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;

		// caret X：测量 caret 前缀宽度（与正文同样的无换行 / 居中段落参数）
		int prefixW = 0;
		if (!prefix.empty())
		{
			auto ctx = TextCodex::Get().BeginDraw();
			Text::RenderRequest& rq = ctx.Request();
			rq.text = prefix;
			rq.canvasMode = Text::CanvasMode::Fixed;
			rq.clearMode = Text::ClearMode::NoClear;
			rq.primaryFont = style_.primaryFont;
			rq.style.fontSize = fontSize;
			rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
			rq.style.wordWrapEnabled = false;
			rq.paddingPx = style_.paddingPx;
			rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
			rq.backgroundColor = Colors::None;

			const Text::MeasureResult measure = ctx.Measure();
			prefixW = static_cast<int>(measure.widthPx) - style_.paddingPx * 2;
		}

		// X = padding + 前缀宽度 + 水平滚动偏移（与正文一致，保证竖线贴在最后一个字后）
		const int caretX = style_.paddingPx + prefixW + static_cast<int>(scrollOffsetX);

		// Y：与垂直居中的正文对齐
		const int caretH = static_cast<int>(fontSize * 1.1f);
		const int canvasH = static_cast<int>(c.GetCanvasHeight());
		const int y0 = std::max(0, (canvasH - caretH) / 2);
		const int y1 = std::min(canvasH - 1, y0 + caretH);

		if (caretX < 0 || static_cast<unsigned>(caretX) >= c.GetCanvasWidth())
			return;

		for (int y = y0; y <= y1; ++y)
			c.PutPixel(static_cast<unsigned>(caretX), static_cast<unsigned>(y), style_.caretColor);
	}

	void TextFieldCanvasView::SyncFrom(const TextFieldViewModel& vm)
	{
		const bool showPlaceholder = vm.textUtf8.empty()
			&& !vm.imeCompositionActive
			&& !vm.placeholderUtf8.empty();
		const std::string displayText = showPlaceholder
			? vm.placeholderUtf8
			: BuildDisplayText_(vm);

		const bool bgDirty = !hasPainted_ || vm.phase != lastPainted_.phase;
		const bool textDirty = !hasPainted_
			|| vm.textUtf8 != lastPainted_.textUtf8
			|| vm.placeholderUtf8 != lastPainted_.placeholderUtf8
			|| vm.imeCompositionUtf8 != lastPainted_.imeCompositionUtf8
			|| vm.imeCompositionActive != lastPainted_.imeCompositionActive
			|| vm.fontSize != lastPainted_.fontSize
			|| IsDisabledPhase(vm.phase) != IsDisabledPhase(lastPainted_.phase);
		const bool ringDirty = !hasPainted_
			|| (vm.phase == UiVisualPhase::Focused) != (lastPainted_.phase == UiVisualPhase::Focused);
		const bool caretDirty = !hasPainted_
			|| vm.caretByteIndex != lastPainted_.caretByteIndex
			|| vm.showCaret != lastPainted_.showCaret
			|| textDirty;

		const float fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;
		const float scrollOffsetX = showPlaceholder
			? 0.0f
			: ComputeScrollOffsetX_(displayText, fontSize);

		if (bgDirty)
			RepaintBackground_(vm.phase);
		if (textDirty)
			RepaintText_(vm, scrollOffsetX);
		if (ringDirty)
			RepaintFocusRing_(vm.phase);
		if (caretDirty)
		{
			const std::size_t caretIndex = showPlaceholder
				? 0u
				: CaretIndexInDisplay_(vm);
			RepaintCaret_(vm, displayText, caretIndex, scrollOffsetX);
		}

		if (bgDirty || textDirty || ringDirty || caretDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
