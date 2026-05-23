#pragma once
#include "IUiView.h"
#include "SliderGrooveLayout.h"
#include "SliderViewModel.h"

#include "Canvas2D.h"
#include "SliderCanvasFill.h"
#include "Colors.h"

#include <memory>

namespace Ui
{
	struct SliderCanvasStyle
	{
		Color trackNormal = Color(45u, 45u, 48u, 255u);
		Color trackFocused = Color(55u, 55u, 60u, 255u);
		Color trackPressed = Color(80u, 80u, 80u, 255u);
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
		void ApplyLayout(const SliderGrooveLayout& layout) noexcept;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

		[[nodiscard]] Canvas2D& GetTrackCanvas() noexcept { return *trackCanvas_; }
		[[nodiscard]] SliderCanvasFill& GetFillCanvas() noexcept { return *fillCanvas_; }

		[[nodiscard]] float GetGrooveInsetLogical() const noexcept
		{
			return static_cast<float>(style_.paddingPx + style_.borderPx);
		}

	private:
		[[nodiscard]] Color TrackColorForPhase(const SliderViewModel& vm) const noexcept;
		[[nodiscard]] Color FillColorForPhase(const SliderViewModel& vm) const noexcept;
		void BakeTrackBorder_();
		void RepaintTrackFill_(const SliderViewModel& vm);
		void ApplyFillParams_(const SliderViewModel& vm);

		SliderCanvasStyle style_;
		std::unique_ptr<Canvas2D> trackCanvas_;
		std::unique_ptr<SliderCanvasFill> fillCanvas_;

		SliderViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}