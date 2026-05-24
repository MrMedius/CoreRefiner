#include "DropdownCanvasComponent.h"

#include "DropdownViewModel.h"
#include "FocusManager.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Ui
{
	namespace
	{
		[[nodiscard]] UiRect RectFromCenterExtents(
			const float cx,
			const float cy,
			const float w,
			const float h) noexcept
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

	DropdownCanvasComponent::DropdownCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const float centerX,
		const float centerY,
		const float width,
		const float headerHeight,
		DropdownCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "DropdownCanvasComponent: invalid FocusHandle");

		constexpr unsigned kMaxCanvasPixelDim = 2048u;
		const unsigned pixelW = std::min(
			kMaxCanvasPixelDim,
			static_cast<unsigned>(std::max(1.0f, width)));
		const unsigned pixelH = std::min(
			kMaxCanvasPixelDim,
			static_cast<unsigned>(std::max(1.0f, headerHeight)));

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		dropdown_ = std::make_unique<UiDropdown>(focusHandle, kPlaceholder);
		view_ = std::make_unique<DropdownCanvasView>(gfx, pixelW, pixelH, std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, width, headerHeight);
	}

	void DropdownCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float headerHeight) noexcept
	{
		if (width <= 0.0f || headerHeight <= 0.0f)
			return;

		dropdown_->SetHeaderBounds(RectFromCenterExtents(centerX, centerY, width, headerHeight));
		dropdown_->SetItemHeight(headerHeight);
		view_->ApplyLayout(centerX, centerY, width, headerHeight);
	}

	void DropdownCanvasComponent::SetOptions(std::vector<DropdownOption> options)
	{
		dropdown_->SetOptions(std::move(options));
	}

	void DropdownCanvasComponent::AddOptions(std::vector<DropdownOption> options)
	{
		dropdown_->AddOptions(std::move(options));
	}

	void DropdownCanvasComponent::AddOption(const DropdownOption option)
	{
		dropdown_->AddOption(std::move(option));
	}

	void DropdownCanvasComponent::EraseOptions(std::vector<int> indices)
	{
		dropdown_->EraseOptions(std::move(indices));
	}

	void DropdownCanvasComponent::EraseOption(const int index)
	{
		dropdown_->EraseOption(index);
	}

	void DropdownCanvasComponent::ClearOptions() noexcept
	{
		dropdown_->ClearOptions();
	}

	void DropdownCanvasComponent::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		dropdown_->Update(frame, focus);
	}

	void DropdownCanvasComponent::SyncView()
	{
		view_->SyncFrom(MakeDropdownViewModel(*dropdown_));
	}

	void DropdownCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)
	{
		view_->LinkTechniques(rg);
	}

	void DropdownCanvasComponent::Submit(const std::size_t channelMask) const
	{
		view_->Submit(channelMask);
	}

	void DropdownCanvasComponent::ResetPointerInteraction() noexcept
	{
		dropdown_->ResetPointerInteraction();
	}

	bool DropdownCanvasComponent::IsFocusable() const noexcept
	{
		return dropdown_->IsFocusable();
	}

	bool DropdownCanvasComponent::ConsumesDirectionalNavigation() const noexcept
	{
		return dropdown_->ConsumesDirectionalNavigation();
	}

	void DropdownCanvasComponent::RegisterTo(UiRoot& root)
	{
		root.AddUiComponent(this);
	}
}
