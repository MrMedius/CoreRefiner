#include "ProgressBarCanvasComponent.h"

#include "FocusManager.h"
#include "ProgressBarViewModel.h"

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

	ProgressBarCanvasComponent::ProgressBarCanvasComponent(
		Graphics& gfx,
		const unsigned canvasPixelWidth,
		const unsigned canvasPixelHeight,
		ProgressBarCanvasStyle style)
	{
		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		progressBar_ = std::make_unique<UiProgressBar>(kPlaceholder);
		view_ = std::make_unique<ProgressBarCanvasView>(gfx, canvasPixelWidth, canvasPixelHeight, std::move(style));
	}

	ProgressBarCanvasComponent::ProgressBarCanvasComponent(
		Graphics& gfx,
		float centerX,
		float centerY,
		float width,
		float height,
		ProgressBarCanvasStyle style)
	{
		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		progressBar_ = std::make_unique<UiProgressBar>(kPlaceholder);
		view_ = std::make_unique<ProgressBarCanvasView>(
			gfx,
			static_cast<unsigned>(std::max(1.0f, width)),
			static_cast<unsigned>(std::max(1.0f, height)),
			std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, width, height);
	}

	void ProgressBarCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		progressBar_->SetBounds(RectFromCenterExtents(centerX, centerY, width, height));

		view_->GetCanvas().SetPosition(DirectX::XMFLOAT3{ centerX, centerY, 0.0f });
		view_->GetCanvas().SetScale(DirectX::XMFLOAT3{ width, height, 1.0f });
	}

	void ProgressBarCanvasComponent::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		progressBar_->Update(frame, focus);
	}

	void ProgressBarCanvasComponent::SyncView()
	{
		view_->SyncFrom(MakeProgressBarViewModel(*progressBar_));
	}

	void ProgressBarCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)
	{
		view_->LinkTechniques(rg);
	}

	void ProgressBarCanvasComponent::Submit(const std::size_t channelMask) const
	{
		view_->Submit(channelMask);
	}

	void ProgressBarCanvasComponent::RegisterTo(UiRoot& root)
	{
		root.AddUiComponent(this);
	}
}
