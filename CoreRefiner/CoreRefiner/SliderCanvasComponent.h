#pragma once

#include "FocusTypes.h"
#include "IUiComponent.h"
#include "SliderAxis.h"
#include "SliderCanvasView.h"
#include "UiSlider.h"
#include "UiRoot.h"
#include "UiTypes.h"

#include <DirectXMath.h>
#include <memory>

class Graphics;

namespace Rgph
{
	class RenderGraph;
}

namespace Ui
{
	class SliderCanvasComponent final : public IUiComponent
	{
	public:
		SliderCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			SliderAxis axis,
			unsigned canvasPixelWidth,
			unsigned canvasPixelHeight,
			bool interactive = true,
			SliderCanvasStyle style = {});

		SliderCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			SliderAxis axis,
			float centerX,
			float centerY,
			float width,
			float height,
			bool interactive = true,
			SliderCanvasStyle style = {});

		SliderCanvasComponent(const SliderCanvasComponent&) = delete;
		SliderCanvasComponent& operator=(const SliderCanvasComponent&) = delete;
		SliderCanvasComponent(SliderCanvasComponent&&) noexcept = default;
		SliderCanvasComponent& operator=(SliderCanvasComponent&&) noexcept = default;
		~SliderCanvasComponent() override = default;

		[[nodiscard]] UiSlider& Slider() noexcept { return *slider_; }
		[[nodiscard]] const UiSlider& Slider() const noexcept { return *slider_; }

		[[nodiscard]] SliderCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const SliderCanvasView& View() const noexcept { return *view_; }

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float height) noexcept;

		void Update(const UiInputFrame& frame, const FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;

		void RegisterTo(UiRoot& root);

	private:
		FocusHandle focusHandle_;
		std::unique_ptr<UiSlider> slider_;
		std::unique_ptr<SliderCanvasView> view_;
	};
}
