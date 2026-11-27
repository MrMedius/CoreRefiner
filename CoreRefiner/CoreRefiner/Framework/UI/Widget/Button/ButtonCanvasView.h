#pragma once

#include "IUiView.h"
#include "ButtonViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"
#include "TextTypes.h"

#include <memory>
#include <string>

namespace Ui
{
	struct ButtonCanvasStyle
	{
		Color bgNormal   = Color(45u,  45u,  48u,  255u);
		Color bgFocused  = Color(35u,  55u,  95u,  255u);
		Color bgPressed  = Color(25u,  110u, 200u, 255u);
		Color bgDisabled = Color(55u,  55u,  55u,  255u);

		Color textNormal   = Colors::White;
		Color textDisabled = Color(130u, 130u, 130u, 255u);

		Color focusRingColor = Color(120u, 200u, 255u, 255u);
		unsigned focusRingThicknessPx = 3u;

		Text::FontSource primaryFont = Text::FontSource::System(L"Segoe UI");
		float fontSize = 20.0f;
		int paddingPx = 8;
	};

	class ButtonCanvasView final : public IUiView
	{
	public:
		ButtonCanvasView(class Graphics& gfx, unsigned pixelWidth, unsigned pixelHeight, ButtonCanvasStyle style = {});

		void SyncFrom(const ButtonViewModel& vm);
		void ApplyLayout(float centerX, float centerY, float width, float height) noexcept;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

		[[nodiscard]] Canvas2D& GetBgCanvas() noexcept { return *bgCanvas_; }
		[[nodiscard]] Canvas2D& GetTextCanvas() noexcept { return *textCanvas_; }
		[[nodiscard]] Canvas2D& GetRingCanvas() noexcept { return *ringCanvas_; }

		[[nodiscard]] Canvas2D& GetCanvas() noexcept { return *bgCanvas_; }
		[[nodiscard]] const Canvas2D& GetCanvas() const noexcept { return *bgCanvas_; }

	private:
		[[nodiscard]] Color BackgroundForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] Color TextColorForPhase(UiVisualPhase phase) const noexcept;
		void RepaintBackground_(UiVisualPhase phase);
		void RepaintText_(const ButtonViewModel& vm);
		void RepaintFocusRing_(UiVisualPhase phase);

		ButtonCanvasStyle style_;
		std::unique_ptr<Canvas2D> bgCanvas_;
		std::unique_ptr<Canvas2D> textCanvas_;
		std::unique_ptr<Canvas2D> ringCanvas_;

		ButtonViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}
