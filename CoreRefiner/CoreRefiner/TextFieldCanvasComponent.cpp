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
		if (field_ == nullptr || view_ == nullptr || !frame.pointer.primaryPressed)
			return;
		if (!field_->IsEnabled() || !focus.IsFocused(focusHandle_))
			return;

		const UiRect& bounds = field_->GetBounds();
		if (!bounds.Contains(frame.pointer.logicalX, frame.pointer.logicalY))
			return;

		const float boundsW = bounds.maxX - bounds.minX;
		const unsigned canvasW = view_->GetPixelWidth();
		if (boundsW <= 0.0f || canvasW == 0u)
			return;

		// 逻辑坐标换成画布像素。多行还会用纵坐标选行。
		const float canvasX = (frame.pointer.logicalX - bounds.minX) * static_cast<float>(canvasW) / boundsW;
		const float boundsH = bounds.maxY - bounds.minY;
		const unsigned canvasH = view_->GetPixelHeight();
		if (boundsH <= 0.0f || canvasH == 0u)
			return;
		const float canvasY = (frame.pointer.logicalY - bounds.minY) * static_cast<float>(canvasH) / boundsH;
		field_->SetCaretByteIndex(view_->HitTestCaretByte(canvasX, canvasY, MakeTextFieldViewModel(*field_)));
	}

	void TextFieldCanvasComponent::SyncView()
	{
		view_->SyncFrom(MakeTextFieldViewModel(*field_));
		// 自动换行的显示行交回逻辑层，下一帧的上下键和到顶到底用同一套行。
		if (field_->IsMultiline() && field_->IsAutoWrap())
			field_->SetVisualLines(view_->BuildVisualLines(field_->GetText(), field_->GetFontSize(), true));
		else
			field_->ClearVisualLines();
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

	bool TextFieldCanvasComponent::ConsumesNavUp() const noexcept
	{
		return field_->ConsumesNavUp();
	}

	bool TextFieldCanvasComponent::ConsumesNavDown() const noexcept
	{
		return field_->ConsumesNavDown();
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
