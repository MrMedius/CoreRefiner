#pragma once
#include "IUiView.h"
#include "ToggleViewModel.h"

#include "Canvas2D.h"
#include "Colors.h"

#include <memory>

namespace Ui
{
	struct ToggleCanvasStyle
	{
		Color boxNormal = Color(45u, 45u, 48u, 255u);
		Color boxFocused = Color(55u, 55u, 60u, 255u);
		Color boxPressed = Color(80u, 80u, 80u, 255u);
		Color boxDisabled = Color(35u, 35u, 38u, 255u);

		Color borderColor = Color(80u, 80u, 85u, 255u);
		Color checkColor = Color(25u, 110u, 200u, 255u);
		Color checkDisabledColor = Color(100u, 100u, 105u, 255u);

		unsigned borderPx = 1u;
		unsigned checkInsetPx = 4u;
	};

	class ToggleCanvasView final : public IUiView
	{
	public:
		ToggleCanvasView(Graphics& gfx, unsigned pixelSize, ToggleCanvasStyle style = {});

		void SyncFrom(const ToggleViewModel& vm);
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;

		[[nodiscard]] Canvas2D& GetBoxCanvas() noexcept { return *boxCanvas_; }
		[[nodiscard]] Canvas2D& GetCheckCanvas() noexcept { return *checkCanvas_; }

	private:
		[[nodiscard]] Color BoxColorForPhase(const ToggleViewModel& vm) const noexcept;
		[[nodiscard]] Color CheckColorForPhase(const ToggleViewModel& vm) const noexcept;
		void RepaintBox_(const ToggleViewModel& vm);
		void RepaintCheck_(const ToggleViewModel& vm);
		void ApplyLayout_(const ToggleViewModel& vm);

		ToggleCanvasStyle style_;
		std::unique_ptr<Canvas2D> boxCanvas_;
		std::unique_ptr<Canvas2D> checkCanvas_;

		ToggleViewModel lastPainted_{};
		bool hasPainted_ = false;
	};
}
