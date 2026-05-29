#include "StepperCanvasComponent.h"

#include "FocusManager.h"
#include "Graphics.h"
#include "StepperViewModel.h"

#include <algorithm>
#include <cassert>

namespace Ui
{
	namespace
	{
		[[nodiscard]] UiRect RectFromCenterExtents(
			const float cx, const float cy,
			const float w,  const float h) noexcept
		{
			const float hw = w * 0.5f;
			const float hh = h * 0.5f;
			return UiRect{ cx - hw, cy - hh, cx + hw, cy + hh };
		}
	}

	StepperCanvasComponent::StepperCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const float centerX,
		const float centerY,
		const float width,
		const float height,
		StepperCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "StepperCanvasComponent: invalid FocusHandle");

		layoutCenterX_ = centerX;
		layoutCenterY_ = centerY;
		layoutWidth_   = width;
		layoutHeight_  = height;

		const float btnW  = std::max(1.0f, height);
		const float textW = std::max(1.0f, width - 2.0f * btnW);
		const auto  pH    = static_cast<unsigned>(btnW);
		const auto  pText = static_cast<unsigned>(textW);

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		stepper_ = std::make_unique<UiStepper>(focusHandle, kPlaceholder, kPlaceholder);
		view_    = std::make_unique<StepperCanvasView>(gfx, pH, pText, pH, std::move(style));

		ApplyLayout_();
	}

	void StepperCanvasComponent::SetCenterVisible(const bool visible) noexcept
	{
		if (centerVisible_ == visible)
			return;
		centerVisible_ = visible;
		ApplyLayout_();
	}

	void StepperCanvasComponent::SetCenterGapWidth(const float gapWidth) noexcept
	{
		centerGapWidth_ = std::max(0.0f, gapWidth);
		ApplyLayout_();
	}

	void StepperCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		layoutCenterX_ = centerX;
		layoutCenterY_ = centerY;
		layoutWidth_   = width;
		layoutHeight_  = height;
		ApplyLayout_();
	}

	void StepperCanvasComponent::ApplyLayout_() noexcept
	{
		if (layoutWidth_ <= 0.0f || layoutHeight_ <= 0.0f)
			return;

		const float btnW = layoutHeight_;
		const float textW = centerVisible_
			? std::max(0.0f, layoutWidth_ - 2.0f * btnW)
			: 0.0f;
		const float gapW = centerVisible_ ? 0.0f : centerGapWidth_;
		const float halfBtn  = btnW * 0.5f;
		const float halfMiddle = (centerVisible_ ? textW : gapW) * 0.5f;

		stepper_->SetMinusBounds(
			RectFromCenterExtents(
				layoutCenterX_ - halfMiddle - halfBtn,
				layoutCenterY_,
				btnW,
				layoutHeight_));
		stepper_->SetPlusBounds(
			RectFromCenterExtents(
				layoutCenterX_ + halfMiddle + halfBtn,
				layoutCenterY_,
				btnW,
				layoutHeight_));

		view_->ApplyLayout(
			layoutCenterX_,
			layoutCenterY_,
			btnW,
			textW,
			gapW,
			layoutHeight_,
			centerVisible_);
	}

	void StepperCanvasComponent::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		stepper_->Update(frame, focus);
	}

	void StepperCanvasComponent::SyncView()
	{
		StepperViewModel vm = MakeStepperViewModel(*stepper_);
		vm.showCenterText  = centerVisible_;
		vm.centerGapWidth  = centerVisible_ ? 0.0f : centerGapWidth_;
		view_->SyncFrom(vm);
	}

	void StepperCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)
	{
		view_->LinkTechniques(rg);
	}

	void StepperCanvasComponent::Submit(const std::size_t channelMask) const
	{
		view_->Submit(channelMask);
	}

	void StepperCanvasComponent::ResetPointerInteraction() noexcept
	{
		stepper_->ResetPointerInteraction();
	}

	bool StepperCanvasComponent::IsFocusable() const noexcept
	{
		return stepper_->IsFocusable();
	}

	void StepperCanvasComponent::RegisterTo(UiRoot& root)
	{
		root.AddUiComponent(this);
	}
}