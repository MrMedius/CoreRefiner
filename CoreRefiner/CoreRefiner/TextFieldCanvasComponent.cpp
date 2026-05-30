#include "TextFieldCanvasComponent.h"

#include "TextFieldViewModel.h"
#include "FocusManager.h"
#include "Graphics.h"

#include <cassert>
#include <cmath>

namespace Ui
{
	namespace
	{
		[[nodiscard]] UiRect RectFromCenterExtents(const float cx, const float cy, const float w, const float h) noexcept
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

	TextFieldCanvasComponent::TextFieldCanvasComponent(
		Graphics& gfx,
		const FocusHandle focusHandle,
		const unsigned canvasPixelWidth,
		const unsigned canvasPixelHeight,
		TextFieldCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "TextFieldCanvasComponent: invalid FocusHandle");

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		field_ = std::make_unique<UiTextField>(focusHandle, kPlaceholder);
		view_ = std::make_unique<TextFieldCanvasView>(gfx, canvasPixelWidth, canvasPixelHeight, std::move(style));
	}

	TextFieldCanvasComponent::TextFieldCanvasComponent(
		Graphics& gfx,
		FocusHandle focusHandle,
		const float centerX,
		const float centerY,
		const float width,
		const float height,
		TextFieldCanvasStyle style)
		:
		focusHandle_(focusHandle)
	{
		assert(focusHandle != kInvalidFocusHandle && "TextFieldCanvasComponent: invalid FocusHandle");

		constexpr UiRect kPlaceholder{ 0.0f, 0.0f, 1.0f, 1.0f };
		field_ = std::make_unique<UiTextField>(focusHandle, kPlaceholder);
		view_ = std::make_unique<TextFieldCanvasView>(
			gfx,
			static_cast<unsigned>(width),
			static_cast<unsigned>(height),
			std::move(style));

		SetLayoutLogicalCenterSize(centerX, centerY, width, height);
	}

	void TextFieldCanvasComponent::SetLayoutLogicalCenterSize(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		if (width <= 0.0f || height <= 0.0f)
			return;

		const UiRect r = RectFromCenterExtents(centerX, centerY, width, height);
		field_->SetBounds(r);
		view_->ApplyLayout(centerX, centerY, width, height);
	}

	void TextFieldCanvasComponent::SetLayoutLogicalRect(const UiRect& r) noexcept
	{
		const float cx = 0.5f * (r.minX + r.maxX);
		const float cy = 0.5f * (r.minY + r.maxY);
		const float w = r.maxX - r.minX;
		const float h = r.maxY - r.minY;
		SetLayoutLogicalCenterSize(cx, cy, w, h);
	}

	void TextFieldCanvasComponent::Update(const UiInputFrame& frame, FocusManager& focus)
	{
		field_->Update(frame, focus);
	}

	void TextFieldCanvasComponent::SyncView()
	{
		view_->SyncFrom(MakeTextFieldViewModel(*field_));
	}

	void TextFieldCanvasComponent::LinkTechniques(Rgph::RenderGraph& rg)
	{
		view_->LinkTechniques(rg);
	}

	void TextFieldCanvasComponent::Submit(const std::size_t channelMask) const
	{
		view_->Submit(channelMask);
	}

	void TextFieldCanvasComponent::ResetPointerInteraction() noexcept
	{
		field_->ResetPointerInteraction();
	}

	bool TextFieldCanvasComponent::IsFocusable() const noexcept
	{
		return field_->IsFocusable();
	}

	bool TextFieldCanvasComponent::ConsumesDirectionalNavigation() const noexcept
	{
		return field_->ConsumesDirectionalNavigation();
	}

	bool TextFieldCanvasComponent::ConsumesTextInput() const noexcept
	{
		return field_->ConsumesTextInput();
	}

	void TextFieldCanvasComponent::RegisterTo(UiRoot& root)
	{
		root.AddUiComponent(this);
	}
}
