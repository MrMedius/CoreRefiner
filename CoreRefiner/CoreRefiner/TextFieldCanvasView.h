#pragma once

#include "IUiView.h"
#include "TextFieldViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"
#include "TextTypes.h"

#include <memory>
#include <string_view>

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
		// 按画布横坐标找最近的字缝，返回正文里的字节下标。点在输入法组合上时停在组合开头。
		[[nodiscard]] std::size_t HitTestCaretByte(float canvasX, const TextFieldViewModel& vm) const;

		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

	private:
		[[nodiscard]] Color BackgroundForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] Color TextColorForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] std::string BuildDisplayText_(const TextFieldViewModel& vm) const;
		[[nodiscard]] std::size_t CaretIndexInDisplay_(const TextFieldViewModel& vm) const;
		// 量一段文字的自然宽度，去掉左右内边距。不按输入栏的框宽排版，超框时滚动和光标才贴得住字。
		[[nodiscard]] int MeasureContentWidth_(std::string_view text, float fontSize) const;
		// 让光标留在可见范围内。字比框窄时不滚动。
		void UpdateScroll_(const std::string& displayText, std::size_t caretIndex, float fontSize);
		// 显示文字里的下标换回正文下标。组合中的字还没进正文。
		[[nodiscard]] std::size_t BufferIndexForDisplay_(const TextFieldViewModel& vm, std::size_t displayIndex) const;

		void RepaintBackground_(UiVisualPhase phase);
		void RepaintText_(const TextFieldViewModel& vm, float scrollOffsetX);
		void RepaintFocusRing_(UiVisualPhase phase);
		void RepaintCaret_(const TextFieldViewModel& vm, std::string_view displayText, std::size_t caretIndex, float scrollOffsetX);

		TextFieldCanvasStyle style_;
		std::unique_ptr<Canvas2D> bgCanvas_;
		std::unique_ptr<Canvas2D> textCanvas_;
		std::unique_ptr<Canvas2D> ringCanvas_;
		std::unique_ptr<Canvas2D> caretCanvas_;

		TextFieldViewModel lastPainted_{};
		bool hasPainted_ = false;
		// 正文横向滚动。负值表示整行往左挪，用来把光标留在框里。
		float scrollOffsetX_ = 0.0f;
	};
}
