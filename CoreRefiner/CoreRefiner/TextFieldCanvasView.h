#pragma once

#include "IUiView.h"
#include "TextFieldViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"
#include "TextTypes.h"

#include <memory>

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

		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

	private:
		[[nodiscard]] Color BackgroundForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] Color TextColorForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] std::string BuildDisplayText_(const TextFieldViewModel& vm) const;
		[[nodiscard]] std::size_t CaretIndexInDisplay_(const TextFieldViewModel& vm) const;

		void RepaintBackground_(UiVisualPhase phase);
		void RepaintText_(const TextFieldViewModel& vm);
		void RepaintFocusRing_(UiVisualPhase phase);
		void RepaintCaret_(const TextFieldViewModel& vm, std::string_view displayText, std::size_t caretIndex);

		TextFieldCanvasStyle style_;
		std::unique_ptr<Canvas2D> bgCanvas_;
		std::unique_ptr<Canvas2D> textCanvas_;
		std::unique_ptr<Canvas2D> ringCanvas_;
		std::unique_ptr<Canvas2D> caretCanvas_;

		TextFieldViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}
