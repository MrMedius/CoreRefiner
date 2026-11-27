#pragma once
#include "FocusTypes.h"
#include "IUiComponent.h"
#include "StepperCanvasView.h"
#include "UiRoot.h"
#include "UiStepper.h"
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
	class UiRoot;

	class StepperCanvasComponent final : public IUiComponent
	{
	public:
		StepperCanvasComponent(
			Graphics& gfx,
			FocusHandle focusHandle,
			float centerX,
			float centerY,
			float width,
			float height,
			StepperCanvasStyle style = {});

		StepperCanvasComponent(const StepperCanvasComponent&) = delete;
		StepperCanvasComponent& operator=(const StepperCanvasComponent&) = delete;
		StepperCanvasComponent(StepperCanvasComponent&&) noexcept = default;
		StepperCanvasComponent& operator=(StepperCanvasComponent&&) noexcept = default;
		~StepperCanvasComponent() override = default;

		[[nodiscard]] FocusHandle GetFocusHandle() const noexcept override { return focusHandle_; }

		[[nodiscard]] UiStepper& Stepper() noexcept { return *stepper_; }
		[[nodiscard]] const UiStepper& Stepper() const noexcept { return *stepper_; }

		[[nodiscard]] StepperCanvasView& View() noexcept { return *view_; }
		[[nodiscard]] const StepperCanvasView& View() const noexcept { return *view_; }

		void SetCenterVisible(bool visible) noexcept;
		[[nodiscard]] bool IsCenterVisible() const noexcept { return centerVisible_; }

		void SetCenterGapWidth(float gapWidth) noexcept;
		[[nodiscard]] float GetCenterGapWidth() const noexcept { return centerGapWidth_; }

		void SetLayoutLogicalCenterSize(float centerX, float centerY, float width, float height) noexcept;

		void Update(const UiInputFrame& frame, FocusManager& focus) override;
		void SyncView() override;
		void LinkTechniques(Rgph::RenderGraph& rg) override;
		void Submit(std::size_t channelMask) const override;
		void ResetPointerInteraction() noexcept override;
		[[nodiscard]] bool IsFocusable() const noexcept override;

		void RegisterTo(UiRoot& root);

	private:
		void ApplyLayout_() noexcept;

		FocusHandle focusHandle_;
		std::unique_ptr<UiStepper>         stepper_;
		std::unique_ptr<StepperCanvasView> view_;

		float layoutCenterX_  = 0.0f;
		float layoutCenterY_  = 0.0f;
		float layoutWidth_    = 1.0f;
		float layoutHeight_   = 1.0f;
		float centerGapWidth_ = 0.0f;
		bool  centerVisible_  = true;
	};
}
