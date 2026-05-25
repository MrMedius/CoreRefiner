#include "SliderCanvasComponent.h"

#include "FocusManager.h"
#include "Graphics.h"
#include "Math.h"
#include "SliderViewModel.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	SliderCanvasComponent::SliderCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const unsigned canvasPixelWidth,
		const unsigned canvasPixelHeight,
		const bool interactive,
		SliderCanvasStyle style)
		:
		focusHandle_(interactive ? focusHandle : kInvalidFocusHandle)
	{
		slider_ = std::make_unique<UiSlider>(focusHandle_, interactive);
		view_ = std::make_unique<SliderCanvasView>(gfx, canvasPixelWidth, canvasPixelHeight, std::move(style));
	}

	SliderCanvasComponent::SliderCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const float centerX,
		const float centerY,
		const float width,
		const float height,
		const bool interactive,
		SliderCanvasStyle style,
		const float rotationDegZ)
		:
		focusHandle_(interactive ? focusHandle : kInvalidFocusHandle)
	{
		slider_ = std::make_unique<UiSlider>(focusHandle_, interactive);
		view_ = std::make_unique<SliderCanvasView>(
			gfx,
			static_cast<unsigned>(std::max(1.0f, width)),
			static_cast<unsigned>(std::max(1.0f, height)),
			std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, width, height, rotationDegZ);
	}

	void SliderCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float height,
		const float rotationDegZ) noexcept
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		const float rotationRadZ = to_rad(rotationDegZ);
		const float inset = view_->GetGrooveInsetLogical();
		const float grooveW = std::max(0.0f, width - 2.0f * inset);
		const float grooveH = std::max(0.0f, height - 2.0f * inset);

		slider_->SetGrooveLayout(SliderGrooveLayout{
			.centerX = centerX,
			.centerY = centerY,
			.outerWidth = width,
			.outerHeight = height,
			.grooveWidth = grooveW,
			.grooveHeight = grooveH,
			.rotationRadZ = rotationRadZ
		});

		view_->ApplyLayout(slider_->GetGrooveLayout());
	}

	void SliderCanvasComponent::Update(const UiInputFrame& frame, FocusManager& focus)																							
	{																							
		slider_->Update(frame, focus);																							
	}

	void SliderCanvasComponent::SyncView()																							
	{																							
		view_->SyncFrom(MakeSliderViewModel(*slider_));																							
	}																							
	
	void SliderCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)																							
	{																							
		view_->LinkTechniques(rg);																							
	}																							
	
	void SliderCanvasComponent::Submit(const std::size_t channelMask) const																							
	{																							
		view_->Submit(channelMask);																							
	}																							
	
	void SliderCanvasComponent::ResetPointerInteraction() noexcept																							
	{																							
		slider_->ResetPointerInteraction();																							
	}																							
	
	bool SliderCanvasComponent::IsFocusable() const noexcept																							
	{																							
		return slider_->IsFocusable();																							
	}																							
	
	void SliderCanvasComponent::RegisterTo(UiRoot& root)																							
	{																							
		root.AddUiComponent(this);																							
	}																							
}