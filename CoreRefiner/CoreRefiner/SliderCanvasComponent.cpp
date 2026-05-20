#include "SliderCanvasComponent.h"

#include "FocusManager.h"
#include "Graphics.h"
#include "SliderViewModel.h"

#include <cassert>
#include <cmath>

namespace Ui
{
	namespace
	{
		[[nodiscard]] UiRect RectFromCenterExtents(float cx, float cy, float w, float h) noexcept
		{
			const float halfW = w * 0.5f;
			const float halfH = h * 0.5f;
			return UiRect{
				.minX = cx - halfW,
				.minY = cy - halfH,
				.maxX = cx + halfW,
				.maxY = cy + halfH
			};
		}
	}

	SliderCanvasComponent::SliderCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const SliderAxis axis,
		const unsigned canvasPixelWidth,
		const unsigned canvasPixelHeight,
		const bool interactive,
		SliderCanvasStyle style)
		:
		focusHandle_(interactive ? focusHandle : kInvalidFocusHandle)
	{
		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		slider_ = std::make_unique<UiSlider>(focusHandle_, kPlaceholder, axis, interactive);
		view_ = std::make_unique<SliderCanvasView>(gfx, canvasPixelWidth, canvasPixelHeight, std::move(style));
	}

	SliderCanvasComponent::SliderCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const SliderAxis axis,
		float centerX,
		float centerY,
		float width,
		float height,
		const bool interactive,
		SliderCanvasStyle style)
		:
		focusHandle_(interactive ? focusHandle : kInvalidFocusHandle)
	{
		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		slider_ = std::make_unique<UiSlider>(focusHandle_, kPlaceholder, axis, interactive);
		view_ = std::make_unique<SliderCanvasView>(
			gfx,
			static_cast<unsigned>(std::max(1.0f, width)),
			static_cast<unsigned>(std::max(1.0f, height)),
			std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, width, height);
	}

	void SliderCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		slider_->SetBounds(RectFromCenterExtents(centerX, centerY, width, height));

		view_->GetTrackCanvas().SetPosition(DirectX::XMFLOAT3{ centerX, centerY, 0.0f });
		view_->GetTrackCanvas().SetScale(DirectX::XMFLOAT3{ width, height, 1.0f });
	}

	void SliderCanvasComponent::Update(const UiInputFrame& frame, const FocusManager& focus)
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
