#pragma once

#include "IUiView.h"
#include "TextFieldViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"
#include "TextTypes.h"

#include <memory>
#include <string_view>
#include <vector>

namespace Ui
{
	struct TextFieldCanvasStyle
	{
		Color bgNormal = Color(35u, 35u, 38u, 255u);
		Color bgFocused = Color(30u, 45u, 70u, 255u);
		Color bgDisabled = Color(50u, 50u, 50u, 255u);

		Color textNormal = Colors::White;
		Color textDisabled = Color(130u, 130u, 130u, 255u);
		Color placeholderColor = Color(150u, 150u, 160u, 255u);
		Color imeCompositionColor = Color(200u, 230u, 255u, 255u);
		Color caretColor = Colors::White;

		Color focusRingColor = Color(120u, 200u, 255u, 255u);
		unsigned focusRingThicknessPx = 2u;

		Text::FontSource primaryFont = Text::FontSource::System(L"Segoe UI");
		float fontSize = 18.0f;
		int paddingPx = 8;
	};

	class TextFieldCanvasView final : public IUiView
	{
	public:
		TextFieldCanvasView(class Graphics& gfx, unsigned pixelWidth, unsigned pixelHeight, TextFieldCanvasStyle style = {});

		void SyncFrom(const TextFieldViewModel& vm);
		void ApplyLayout(float centerX, float centerY, float width, float height) noexcept;

		// 画布像素宽。点击的横坐标先换算到这个宽度再定位。
		[[nodiscard]] unsigned GetPixelWidth() const noexcept;
		// 画布像素高。多行点击的纵坐标先换算到这个高度再选行。
		[[nodiscard]] unsigned GetPixelHeight() const noexcept;
		// 按画布坐标找最近的字缝，返回正文里的字节下标。单行只用横坐标。点在输入法组合上时停在组合开头。
		[[nodiscard]] std::size_t HitTestCaretByte(float canvasX, float canvasY, const TextFieldViewModel& vm) const;
		// 按框的内宽拆出显示行。autoWrap 为真时超宽另起一行，正文里不插入换行符。
		[[nodiscard]] std::vector<EditLineSpan> BuildVisualLines(std::string_view text, float fontSize, bool autoWrap) const;

		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

	private:
		[[nodiscard]] Color BackgroundForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] Color TextColorForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] std::string BuildDisplayText_(const TextFieldViewModel& vm) const;
		[[nodiscard]] std::size_t CaretIndexInDisplay_(const TextFieldViewModel& vm) const;
		// 一行的起止字节。end 不含换行符，自动换行时也不含折到下一行的字。
		// 按换行和自动换行拆开后的排版。光标前缀宽和当前行宽用来摆光标、做滚动。
		struct TextLayout_
		{
			std::vector<EditLineSpan> lines;
			int lineHeight = 1;
			int caretLine = 0;
			int caretPrefixW = 0;
			int caretLineW = 0;
		};

		// 量一段文字的自然宽度，去掉左右内边距。不按输入栏的框宽排版，超框时滚动和光标才贴得住字。
		[[nodiscard]] int MeasureContentWidth_(std::string_view text, float fontSize) const;
		// 一行的像素高。多行的行距和光标高度都用这个，字和光标才落在同一行。
		[[nodiscard]] int MeasureLineHeight_(float fontSize) const;
		// 按换行拆行。autoWrap 为真时，一行超过内宽就再拆开，不往正文里写换行符。
		[[nodiscard]] std::vector<EditLineSpan> SplitVisualLines_(std::string_view text, float fontSize, bool autoWrap) const;
		// 按显示行排版，并算出光标在哪一行、离行首多远。
		[[nodiscard]] TextLayout_ LayoutText_(std::string_view text, std::size_t caretIndex, float fontSize, bool autoWrap) const;
		// 单行：让光标留在可见范围内。字比框窄时不滚动。
		void UpdateScroll_(const std::string& displayText, std::size_t caretIndex, float fontSize);
		// 多行：当前行比框宽就左右挪，行数比框高就上下挪，让光标这一行完整留在框里。
		void UpdateScrollMultiline_(const TextLayout_& layout);
		// 显示文字里的下标换回正文下标。组合中的字还没进正文。
		[[nodiscard]] std::size_t BufferIndexForDisplay_(const TextFieldViewModel& vm, std::size_t displayIndex) const;

		void RepaintBackground_(UiVisualPhase phase);
		void RepaintText_(const TextFieldViewModel& vm, float scrollOffsetX);
		// 多行时按行从左上角往下画。placeholder 为真时用占位色。行距跟 LayoutText_ 相同。
		void RepaintMultilineText_(const TextFieldViewModel& vm, std::string_view displayText, const TextLayout_& layout, bool placeholder);
		void RepaintFocusRing_(UiVisualPhase phase);
		void RepaintCaret_(const TextFieldViewModel& vm, std::string_view displayText, std::size_t caretIndex, float scrollOffsetX);
		// 多行光标画在当前行、当前字的位置，不居中到整块文字。
		void RepaintMultilineCaret_(const TextFieldViewModel& vm, const TextLayout_& layout);

		TextFieldCanvasStyle style_;
		std::unique_ptr<Canvas2D> bgCanvas_;
		std::unique_ptr<Canvas2D> textCanvas_;
		std::unique_ptr<Canvas2D> ringCanvas_;
		std::unique_ptr<Canvas2D> caretCanvas_;

		TextFieldViewModel lastPainted_{};
		bool hasPainted_ = false;
		// 正文横向滚动。负值表示整行往左挪，用来把光标留在框里。
		float scrollOffsetX_ = 0.0f;
		// 多行纵向滚动。负值表示整段往上挪，用来把光标所在行留在框里。
		float scrollOffsetY_ = 0.0f;
	};
}
