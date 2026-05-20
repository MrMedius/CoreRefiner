#include "ButtonCanvasComponent.h"

#include "ButtonViewModel.h"
#include "FocusManager.h"
#include "Graphics.h"

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

	ButtonCanvasComponent::ButtonCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const unsigned canvasPixelWidth,
		const unsigned canvasPixelHeight,
		ButtonCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "ButtonCanvasComponent: invalid FocusHandle");

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		button_ = std::make_unique<UiButton>(focusHandle, kPlaceholder);
		view_ = std::make_unique<ButtonCanvasView>(gfx, canvasPixelWidth, canvasPixelHeight, std::move(style));
	}

	ButtonCanvasComponent::ButtonCanvasComponent(
		Graphics& gfx,
		FocusHandle focusHandle,
		float centerX, float centerY,
		float width, float height,
		ButtonCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "ButtonCanvasComponent: invalid FocusHandle");

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		button_ = std::make_unique<UiButton>(focusHandle, kPlaceholder);
		view_ = std::make_unique<ButtonCanvasView>(gfx, static_cast<unsigned>(width), static_cast<unsigned>(height), std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, width, height);
	}

	void ButtonCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		const UiRect r = RectFromCenterExtents(centerX, centerY, width, height);
		button_->SetBounds(r);

		view_->GetCanvas().SetPosition(DirectX::XMFLOAT3{ centerX, centerY, 0.0f });
		view_->GetCanvas().SetScale(DirectX::XMFLOAT3{ width, height, 1.0f });
	}

	void ButtonCanvasComponent::SetLayoutLogicalRect(const UiRect& r) noexcept
	{
		const float cx = 0.5f * (r.minX + r.maxX);
		const float cy = 0.5f * (r.minY + r.maxY);
		const float w = r.maxX - r.minX;
		const float h = r.maxY - r.minY;
		SetLayoutLogicalCenterSize(cx, cy, w, h);
	}

	void ButtonCanvasComponent::Update(const UiInputFrame& frame, const FocusManager& focus)
	{
		button_->Update(frame, focus);
	}

	void ButtonCanvasComponent::SyncView()
	{
		view_->SyncFrom(MakeButtonViewModel(*button_));
	}

	void ButtonCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)
	{
		view_->LinkTechniques(rg);
	}

	void ButtonCanvasComponent::Submit(const std::size_t channelMask) const
	{
		view_->Submit(channelMask);
	}

	void ButtonCanvasComponent::ResetPointerInteraction() noexcept
	{
		button_->ResetPointerInteraction();
	}

	bool ButtonCanvasComponent::IsFocusable() const noexcept
	{
		return button_->IsFocusable();
	}

	void ButtonCanvasComponent::RegisterTo(UiRoot& root)
	{
		root.AddUiComponent(this);
	}
}
