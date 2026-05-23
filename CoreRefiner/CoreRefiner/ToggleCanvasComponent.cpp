#include "ToggleCanvasComponent.h"
#include "FocusManager.h"
#include "ToggleViewModel.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Ui
{
	namespace
	{
		[[nodiscard]] UiRect SquareFromCenterSize(const float cx, const float cy, const float size) noexcept
		{
			const float half = size * 0.5f;
			return UiRect{
				.minX = cx - half,
				.minY = cy - half,
				.maxX = cx + half,
				.maxY = cy + half
			};
		}
	}

	ToggleCanvasComponent::ToggleCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const unsigned pixelSize,
		ToggleCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "ToggleCanvasComponent: invalid FocusHandle");

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		toggle_ = std::make_unique<UiToggle>(focusHandle, kPlaceholder);
		view_ = std::make_unique<ToggleCanvasView>(gfx, pixelSize, std::move(style));
	}

	ToggleCanvasComponent::ToggleCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const float centerX,
		const float centerY,
		const float size,
		ToggleCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "ToggleCanvasComponent: invalid FocusHandle");

		const unsigned pixelSize = static_cast<unsigned>(std::max(1.0f, size));
		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		toggle_ = std::make_unique<UiToggle>(focusHandle, kPlaceholder);
		view_ = std::make_unique<ToggleCanvasView>(gfx, pixelSize, std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, size);
	}

	void ToggleCanvasComponent::ApplyViewLayout_() noexcept
	{
		view_->ApplyLayout(layoutCenterX_, layoutCenterY_, layoutSize_, layoutBoxScale_);
	}

	void ToggleCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float size) noexcept
	{
		if (size <= 0.0f)
			return;

		layoutCenterX_ = centerX;
		layoutCenterY_ = centerY;
		layoutSize_ = size;
		toggle_->SetBounds(SquareFromCenterSize(centerX, centerY, size));
		ApplyViewLayout_();
	}

	void ToggleCanvasComponent::SetLayoutLogicalRect(const UiRect& r) noexcept
	{
		const float cx = 0.5f * (r.minX + r.maxX);
		const float cy = 0.5f * (r.minY + r.maxY);
		const float size = std::max(r.maxX - r.minX, r.maxY - r.minY);
		SetLayoutLogicalCenterSize(cx, cy, size);
	}

	void ToggleCanvasComponent::SetLayoutBoxScale(const float scale) noexcept
	{
		layoutBoxScale_ = scale;
		ApplyViewLayout_();
	}

	void ToggleCanvasComponent::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		toggle_->Update(frame, focus);
	}

	void ToggleCanvasComponent::SyncView()
	{
		view_->SyncFrom(MakeToggleViewModel(*toggle_));
	}

	void ToggleCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)
	{
		view_->LinkTechniques(rg);
	}

	void ToggleCanvasComponent::Submit(const std::size_t channelMask) const
	{
		view_->Submit(channelMask);
	}

	void ToggleCanvasComponent::ResetPointerInteraction() noexcept
	{
		toggle_->ResetPointerInteraction();
	}

	bool ToggleCanvasComponent::IsFocusable() const noexcept
	{
		return toggle_->IsFocusable();
	}

	void ToggleCanvasComponent::RegisterTo(UiRoot& root)
	{
		root.AddUiComponent(this);
	}
}
