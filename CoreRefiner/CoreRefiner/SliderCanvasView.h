#pragma once

#include "IUiView.h"
#include "SliderViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"

#include <memory>

namespace Ui
{
	struct SliderCanvasStyle
	{
		Color trackNormal = Color(45u, 45u, 48u, 255u);
		Color trackFocused = Color(55u, 55u, 60u, 255u);
		Color trackPressed = Color(40u, 40u, 44u, 255u);
		Color trackDisabled = Color(35u, 35u, 38u, 255u);

		Color fillNormal = Color(25u, 110u, 200u, 255u);
		Color fillDisabled = Color(80u, 80u, 85u, 255u);

		Color borderColor = Color(80u, 80u, 85u, 255u);
		unsigned paddingPx = 4u;
		unsigned borderPx = 1u;
	};

	class SliderCanvasView final : public IUiView
	{
	public:
		SliderCanvasView(Graphics& gfx, unsigned pixelWidth, unsigned pixelHeight, SliderCanvasStyle style = {});

		void SyncFrom(const SliderViewModel& vm);
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

		[[nodiscard]] Canvas2D& GetTrackCanvas() noexcept { return *trackCanvas_; }
		[[nodiscard]] Canvas2D& GetFillCanvas() noexcept { return *fillCanvas_; }

	private:
		[[nodiscard]] Color TrackColorForPhase(const SliderViewModel& vm) const noexcept;
		[[nodiscard]] Color FillColorForPhase(const SliderViewModel& vm) const noexcept;
		void RepaintTrack_(const SliderViewModel& vm);
		void ApplyFillTransform_(const SliderViewModel& vm);

		SliderCanvasStyle style_;
		std::unique_ptr<Canvas2D> trackCanvas_;
		std::unique_ptr<Canvas2D> fillCanvas_;

		SliderViewModel lastPainted_{};
		bool hasPainted_ = false;
		bool fillInitialized_ = false;
	};
}
