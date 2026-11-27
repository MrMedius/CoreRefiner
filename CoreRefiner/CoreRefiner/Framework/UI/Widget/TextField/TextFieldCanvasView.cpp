#include "TextFieldCanvasView.h"

#include "CanvasPixelDraw.h"
#include "Channels.h"
#include "Graphics.h"
#include "RenderGraph.h"
#include "TextCodex.h"
#include "Util.h"

#include <algorithm>
#include <cstdlib>

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

	unsigned TextFieldCanvasView::GetPixelWidth() const noexcept
	{
		return textCanvas_->GetCanvasWidth();
	}

	unsigned TextFieldCanvasView::GetPixelHeight() const noexcept
	{
		return textCanvas_->GetCanvasHeight();
	}

	int TextFieldCanvasView::MeasureContentWidth_(const std::string_view text, const float fontSize) const
	{
		if (text.empty())
			return 0;

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = std::string(text);
		// 自动宽度且不换行：按字的自然宽度量，不被输入栏的框宽截断。
		rq.canvasMode = Text::CanvasMode::Auto;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = style_.paddingPx;
		rq.backgroundColor = Colors::None;

		const Text::MeasureResult measure = ctx.Measure();
		const int width = static_cast<int>(measure.widthPx) - style_.paddingPx * 2;
		return (width > 0) ? width : 0;
	}

	int TextFieldCanvasView::MeasureLineHeight_(const float fontSize) const
	{
		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = "Ag";
		rq.canvasMode = Text::CanvasMode::Auto;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = 0;
		rq.backgroundColor = Colors::None;

		const int height = static_cast<int>(ctx.Measure().heightPx);
		return (height > 0) ? height : static_cast<int>(fontSize);
	}

	std::vector<EditLineSpan> TextFieldCanvasView::SplitVisualLines_(
		const std::string_view text,
		const float fontSize,
		const bool autoWrap) const
	{
		std::vector<EditLineSpan> lines;
		const int canvasW = static_cast<int>(textCanvas_->GetCanvasWidth());
		const int innerW = canvasW - style_.paddingPx * 2;
		const bool wrap = autoWrap && innerW > 0;

		auto pushHardLine = [&](const std::size_t hardBegin, const std::size_t hardEnd)
		{
			if (!wrap || hardBegin >= hardEnd)
			{
				lines.push_back(EditLineSpan{ hardBegin, hardEnd });
				return;
			}

			std::size_t lineBegin = hardBegin;
			while (lineBegin < hardEnd)
			{
				std::size_t index = lineBegin;
				std::size_t lineEnd = lineBegin;
				while (index < hardEnd)
				{
					const std::size_t next = Utf8Next(text, index);
					if (next <= index)
						break;
					const std::size_t clipped = (next > hardEnd) ? hardEnd : next;
					const int width = MeasureContentWidth_(text.substr(lineBegin, clipped - lineBegin), fontSize);
					// 至少留下一个码点，单个字比框宽时仍靠左右滚动。
					if (width > innerW && lineEnd > lineBegin)
						break;
					lineEnd = clipped;
					index = clipped;
				}
				if (lineEnd <= lineBegin)
					break;
				lines.push_back(EditLineSpan{ lineBegin, lineEnd });
				lineBegin = lineEnd;
			}
		};

		if (text.empty())
		{
			lines.push_back(EditLineSpan{ 0u, 0u });
			return lines;
		}

		std::size_t i = 0u;
		while (true)
		{
			const std::size_t begin = i;
			while (i < text.size() && text[i] != '\n')
				i = Utf8Next(text, i);
			pushHardLine(begin, i);
			if (i >= text.size())
				break;
			// 跳过真正的换行符。末尾再来一个换行时，后面补一行空行给光标。
			++i;
			if (i == text.size())
			{
				lines.push_back(EditLineSpan{ i, i });
				break;
			}
		}
		return lines;
	}

	std::vector<EditLineSpan> TextFieldCanvasView::BuildVisualLines(
		const std::string_view text,
		const float fontSize,
		const bool autoWrap) const
	{
		const float size = fontSize > 0.0f ? fontSize : style_.fontSize;
		return SplitVisualLines_(text, size, autoWrap);
	}

	TextFieldCanvasView::TextLayout_ TextFieldCanvasView::LayoutText_(
		const std::string_view text,
		std::size_t caretIndex,
		const float fontSize,
		const bool autoWrap) const
	{
		TextLayout_ out;
		out.lineHeight = MeasureLineHeight_(fontSize);
		if (out.lineHeight < 1)
			out.lineHeight = 1;
		out.lines = SplitVisualLines_(text, fontSize, autoWrap);

		if (caretIndex > text.size())
			caretIndex = text.size();
		if (out.lines.empty())
			out.lines.push_back(EditLineSpan{ 0u, 0u });

		out.caretLine = static_cast<int>(out.lines.size()) - 1;
		for (int n = 0; n < static_cast<int>(out.lines.size()); ++n)
		{
			const bool last = (n + 1 == static_cast<int>(out.lines.size()));
			if (caretIndex < out.lines[static_cast<std::size_t>(n)].begin)
				break;
			if (last || caretIndex <= out.lines[static_cast<std::size_t>(n)].end)
			{
				out.caretLine = n;
				break;
			}
		}

		const EditLineSpan& line = out.lines[static_cast<std::size_t>(out.caretLine)];
		const std::size_t caretOnLine = (caretIndex < line.begin)
			? line.begin
			: ((caretIndex > line.end) ? line.end : caretIndex);
		out.caretPrefixW = MeasureContentWidth_(text.substr(line.begin, caretOnLine - line.begin), fontSize);
		out.caretLineW = MeasureContentWidth_(text.substr(line.begin, line.end - line.begin), fontSize);
		return out;
	}

	void TextFieldCanvasView::UpdateScrollMultiline_(const TextLayout_& layout)
	{
		const int canvasW = static_cast<int>(textCanvas_->GetCanvasWidth());
		const int canvasH = static_cast<int>(textCanvas_->GetCanvasHeight());
		const int innerW = canvasW - style_.paddingPx * 2;
		const int innerH = canvasH - style_.paddingPx * 2;
		if (innerW <= 0 || innerH <= 0)
		{
			scrollOffsetX_ = 0.0f;
			scrollOffsetY_ = 0.0f;
			return;
		}

		// 当前这一行比框宽时，左右挪，让光标落在框内。
		if (layout.caretLineW <= innerW)
			scrollOffsetX_ = 0.0f;
		else
		{
			const float visibleLeft = -scrollOffsetX_;
			const float visibleRight = visibleLeft + static_cast<float>(innerW);
			if (static_cast<float>(layout.caretPrefixW) < visibleLeft)
				scrollOffsetX_ = -static_cast<float>(layout.caretPrefixW);
			else if (static_cast<float>(layout.caretPrefixW) > visibleRight)
				scrollOffsetX_ = static_cast<float>(innerW) - static_cast<float>(layout.caretPrefixW);

			const float minScroll = static_cast<float>(innerW - layout.caretLineW);
			if (scrollOffsetX_ > 0.0f)
				scrollOffsetX_ = 0.0f;
			if (scrollOffsetX_ < minScroll)
				scrollOffsetX_ = minScroll;
		}

		// 行数比框高时，上下挪，让光标所在的整行留在框里。
		const int totalH = static_cast<int>(layout.lines.size()) * layout.lineHeight;
		if (totalH <= innerH)
			scrollOffsetY_ = 0.0f;
		else
		{
			const float caretTop = static_cast<float>(layout.caretLine * layout.lineHeight);
			const float caretBottom = caretTop + static_cast<float>(layout.lineHeight);
			const float visibleTop = -scrollOffsetY_;
			const float visibleBottom = visibleTop + static_cast<float>(innerH);
			if (caretTop < visibleTop)
				scrollOffsetY_ = -caretTop;
			else if (caretBottom > visibleBottom)
			{
				// 这一行比框还高时钉住行首，避免为了同时露出行尾而上下来回跳。
				if (layout.lineHeight >= innerH)
					scrollOffsetY_ = -caretTop;
				else
					scrollOffsetY_ = static_cast<float>(innerH) - caretBottom;
			}

			const float minScroll = static_cast<float>(innerH - totalH);
			if (scrollOffsetY_ > 0.0f)
				scrollOffsetY_ = 0.0f;
			if (scrollOffsetY_ < minScroll)
				scrollOffsetY_ = minScroll;
		}
	}

	void TextFieldCanvasView::UpdateScroll_(
		const std::string& displayText,
		const std::size_t caretIndex,
		const float fontSize)
	{
		const int canvasW = static_cast<int>(textCanvas_->GetCanvasWidth());
		const int innerW = canvasW - style_.paddingPx * 2;
		if (innerW <= 0 || displayText.empty())
		{
			scrollOffsetX_ = 0.0f;
			scrollOffsetY_ = 0.0f;
			return;
		}

		const int textW = MeasureContentWidth_(displayText, fontSize);
		if (textW <= innerW)
		{
			scrollOffsetX_ = 0.0f;
			return;
		}

		const std::size_t clamped = (std::min)(caretIndex, displayText.size());
		const int prefixW = MeasureContentWidth_(std::string_view(displayText).substr(0, clamped), fontSize);
		const float visibleLeft = -scrollOffsetX_;
		const float visibleRight = visibleLeft + static_cast<float>(innerW);
		if (static_cast<float>(prefixW) < visibleLeft)
			scrollOffsetX_ = -static_cast<float>(prefixW);
		else if (static_cast<float>(prefixW) > visibleRight)
			scrollOffsetX_ = static_cast<float>(innerW) - static_cast<float>(prefixW);

		const float minScroll = static_cast<float>(innerW - textW);
		if (scrollOffsetX_ > 0.0f)
			scrollOffsetX_ = 0.0f;
		if (scrollOffsetX_ < minScroll)
			scrollOffsetX_ = minScroll;
	}

	std::size_t TextFieldCanvasView::BufferIndexForDisplay_(
		const TextFieldViewModel& vm,
		const std::size_t displayIndex) const
	{
		if (!vm.imeCompositionActive || vm.imeCompositionUtf8.empty())
			return (std::min)(displayIndex, vm.textUtf8.size());

		const std::size_t compStart = (std::min)(vm.caretByteIndex, vm.textUtf8.size());
		const std::size_t compEnd = compStart + vm.imeCompositionUtf8.size();
		if (displayIndex <= compStart)
			return displayIndex;
		// 组合中的字还没进正文，点在这段上就停在组合开头。
		if (displayIndex < compEnd)
			return compStart;

		const std::size_t after = displayIndex - vm.imeCompositionUtf8.size();
		return (std::min)(after, vm.textUtf8.size());
	}

	std::size_t TextFieldCanvasView::HitTestCaretByte(
		const float canvasX,
		const float canvasY,
		const TextFieldViewModel& vm) const
	{
		const bool showPlaceholder = vm.textUtf8.empty()
			&& !vm.imeCompositionActive
			&& !vm.placeholderUtf8.empty();
		if (showPlaceholder)
			return 0u;

		const std::string displayText = BuildDisplayText_(vm);
		if (displayText.empty())
			return 0u;

		const float fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;
		const float contentX = canvasX - static_cast<float>(style_.paddingPx) - scrollOffsetX_;

		if (!vm.multiline)
		{
			std::size_t best = 0u;
			int bestDist = std::abs(static_cast<int>(contentX));
			std::size_t index = 0u;
			while (index < displayText.size())
			{
				index = Utf8Next(displayText, index);
				const int width = MeasureContentWidth_(std::string_view(displayText).substr(0, index), fontSize);
				const int dist = std::abs(width - static_cast<int>(contentX));
				if (dist < bestDist)
				{
					bestDist = dist;
					best = index;
				}
			}
			return BufferIndexForDisplay_(vm, best);
		}

		const TextLayout_ layout = LayoutText_(displayText, 0u, fontSize, vm.autoWrap);
		if (layout.lines.empty() || layout.lineHeight <= 0)
			return 0u;

		const float contentY = canvasY - static_cast<float>(style_.paddingPx) - scrollOffsetY_;
		int line = 0;
		if (contentY > 0.0f)
			line = static_cast<int>(contentY / static_cast<float>(layout.lineHeight));
		if (line < 0)
			line = 0;
		if (line >= static_cast<int>(layout.lines.size()))
			line = static_cast<int>(layout.lines.size()) - 1;

		const EditLineSpan& span = layout.lines[static_cast<std::size_t>(line)];
		const std::string_view slice = std::string_view(displayText).substr(span.begin, span.end - span.begin);
		std::size_t best = span.begin;
		int bestDist = std::abs(static_cast<int>(contentX));
		std::size_t local = 0u;
		while (local < slice.size())
		{
			local = Utf8Next(slice, local);
			const int width = MeasureContentWidth_(slice.substr(0, local), fontSize);
			const int dist = std::abs(width - static_cast<int>(contentX));
			if (dist < bestDist)
			{
				bestDist = dist;
				best = span.begin + local;
			}
		}
		return BufferIndexForDisplay_(vm, best);
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

	void TextFieldCanvasView::RepaintMultilineText_(
		const TextFieldViewModel& vm,
		const std::string_view displayText,
		const TextLayout_& layout,
		const bool placeholder)
	{
		Canvas& c = *textCanvas_;
		c.Clear(Colors::None);

		const float fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;
		const int canvasH = static_cast<int>(c.GetCanvasHeight());
		for (int n = 0; n < static_cast<int>(layout.lines.size()); ++n)
		{
			const EditLineSpan& line = layout.lines[static_cast<std::size_t>(n)];
			if (line.end <= line.begin)
				continue;

			const float y = static_cast<float>(style_.paddingPx)
				+ static_cast<float>(n * layout.lineHeight)
				+ scrollOffsetY_;
			if (y + static_cast<float>(layout.lineHeight) < 0.0f || y > static_cast<float>(canvasH))
				continue;

			auto ctx = TextCodex::Get().BeginDraw();
			Text::RenderRequest& rq = ctx.Request();
			rq.text = std::string(displayText.substr(line.begin, line.end - line.begin));
			rq.canvasMode = Text::CanvasMode::Fixed;
			rq.clearMode = Text::ClearMode::NoClear;
			rq.primaryFont = style_.primaryFont;
			rq.style.fontSize = fontSize;
			rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
			rq.style.wordWrapEnabled = false;
			rq.paddingPx = 0;
			// 这一行已经按显示行拆好。槽位给得很宽，剩下的超宽靠滚动。
			rq.SetDestRect(
				static_cast<float>(style_.paddingPx) + scrollOffsetX_,
				y,
				100000.0f,
				static_cast<float>(layout.lineHeight));
			rq.defaultColor = placeholder ? style_.placeholderColor : TextColorForPhase(vm.phase);
			rq.backgroundColor = Colors::None;
			ctx.Render(c);
		}
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
		const int prefixW = MeasureContentWidth_(prefix, fontSize);

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

	void TextFieldCanvasView::RepaintMultilineCaret_(
		const TextFieldViewModel& vm,
		const TextLayout_& layout)
	{
		Canvas& c = *caretCanvas_;
		c.Clear(Colors::None);

		if (!vm.showCaret || vm.phase != UiVisualPhase::Focused)
			return;

		const float fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;
		const int caretX = style_.paddingPx + layout.caretPrefixW + static_cast<int>(scrollOffsetX_);
		const int lineTop = style_.paddingPx
			+ layout.caretLine * layout.lineHeight
			+ static_cast<int>(scrollOffsetY_);
		const int caretH = std::max(1, static_cast<int>(fontSize * 1.1f));
		const int canvasH = static_cast<int>(c.GetCanvasHeight());
		const int y0 = std::max(0, lineTop + std::max(0, (layout.lineHeight - caretH) / 2));
		const int y1 = std::min(canvasH - 1, y0 + caretH);

		if (caretX < 0 || static_cast<unsigned>(caretX) >= c.GetCanvasWidth() || y0 > y1)
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
			|| vm.multiline != lastPainted_.multiline
			|| vm.autoWrap != lastPainted_.autoWrap
			|| IsDisabledPhase(vm.phase) != IsDisabledPhase(lastPainted_.phase);
		const bool ringDirty = !hasPainted_
			|| (vm.phase == UiVisualPhase::Focused) != (lastPainted_.phase == UiVisualPhase::Focused);

		const float fontSize = vm.fontSize > 0.0f ? vm.fontSize : style_.fontSize;
		const float scrollBeforeX = scrollOffsetX_;
		const float scrollBeforeY = scrollOffsetY_;
		TextLayout_ layout{};
		if (vm.multiline)
		{
			// 空栏也按多行排。占位字和光标从第一行左上角开始，不拿到整框正中。
			const std::size_t caretIndex = showPlaceholder ? 0u : CaretIndexInDisplay_(vm);
			layout = LayoutText_(displayText, caretIndex, fontSize, vm.autoWrap);
			if (showPlaceholder)
			{
				scrollOffsetX_ = 0.0f;
				scrollOffsetY_ = 0.0f;
			}
			else
				UpdateScrollMultiline_(layout);
		}
		else if (showPlaceholder)
		{
			scrollOffsetX_ = 0.0f;
			scrollOffsetY_ = 0.0f;
		}
		else
		{
			scrollOffsetY_ = 0.0f;
			UpdateScroll_(displayText, CaretIndexInDisplay_(vm), fontSize);
		}
		const bool scrollDirty = scrollOffsetX_ != scrollBeforeX || scrollOffsetY_ != scrollBeforeY;
		const bool caretDirty = !hasPainted_
			|| vm.caretByteIndex != lastPainted_.caretByteIndex
			|| vm.showCaret != lastPainted_.showCaret
			|| textDirty
			|| scrollDirty;

		if (bgDirty)
			RepaintBackground_(vm.phase);
		if (textDirty || scrollDirty)
		{
			if (vm.multiline)
				RepaintMultilineText_(vm, displayText, layout, showPlaceholder);
			else
				RepaintText_(vm, scrollOffsetX_);
		}
		if (ringDirty)
			RepaintFocusRing_(vm.phase);
		if (caretDirty)
		{
			if (vm.multiline)
				RepaintMultilineCaret_(vm, layout);
			else
			{
				const std::size_t caretIndex = showPlaceholder
					? 0u
					: CaretIndexInDisplay_(vm);
				RepaintCaret_(vm, displayText, caretIndex, scrollOffsetX_);
			}
		}

		if (bgDirty || textDirty || ringDirty || caretDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
