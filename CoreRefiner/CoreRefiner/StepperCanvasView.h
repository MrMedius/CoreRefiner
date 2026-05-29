#pragma once
#include "IUiView.h"
#include "StepperViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"
#include "TextTypes.h"

#include <memory>

namespace Ui
{
	struct StepperCanvasStyle
	{
		Color btnBgNormal   = Color(45u,  45u,  48u,  255u);
		Color btnBgFocused  = Color(35u,  55u,  95u,  255u);
		Color btnBgPressed  = Color(25u,  110u, 200u, 255u);
		Color btnBgDisabled = Color(55u,  55u,  55u,  255u);

		Color textBgNormal   = Color(30u, 30u, 32u, 255u);
		Color textBgDisabled = Color(40u, 40u, 40u, 255u);

		Color symbolNormal   = Colors::White;
		Color symbolDisabled = Color(130u, 130u, 130u, 255u);
		Color textNormal     = Colors::White;
		Color textDisabled   = Color(130u, 130u, 130u, 255u);

		Color    focusRingColor       = Color(120u, 200u, 255u, 255u);
		unsigned focusRingThicknessPx = 3u;

		Text::FontSource primaryFont = Text::FontSource::System(L"Segoe UI");
		float fontSize  = 20.0f;
		int   paddingPx = 4;
	};

	class StepperCanvasView final : public IUiView
	{
	public:
		StepperCanvasView(
			Graphics& gfx,
			unsigned btnPixelSize,
			unsigned textPixelW,
			unsigned pixelH,
			StepperCanvasStyle style = {});

		void ApplyLayout(
			float totalCenterX, float totalCenterY,
			float buttonWidth, float textWidth, float centerGapWidth,
			float height, bool showCenterText) noexcept;

		void SyncFrom(const StepperViewModel& vm);

		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

	private:
		[[nodiscard]] Color BtnBgForPhase(UiVisualPhase phase) const noexcept;
		[[nodiscard]] Color SymbolColorForEnabled(bool enabled) const noexcept;
		[[nodiscard]] Color TextColorForEnabled(bool enabled) const noexcept;

		void RepaintMinusBg_(UiVisualPhase phase);
		void RepaintPlusBg_(UiVisualPhase phase);
		void RepaintMinusArrow_(bool enabled);
		void RepaintPlusArrow_(bool enabled);
		void RepaintText_(const StepperViewModel& vm);
		void RepaintRing_(bool show);

		StepperCanvasStyle style_;

		std::unique_ptr<Canvas2D> minusBgCanvas_;
		std::unique_ptr<Canvas2D> minusArrowCanvas_;
		std::unique_ptr<Canvas2D> textCanvas_;
		std::unique_ptr<Canvas2D> plusBgCanvas_;
		std::unique_ptr<Canvas2D> plusArrowCanvas_;
		std::unique_ptr<Canvas2D> ringCanvas_;

		bool showCenterText_ = true;
		StepperViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}
