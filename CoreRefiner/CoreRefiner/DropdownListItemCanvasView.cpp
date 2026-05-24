#include "DropdownListItemCanvasView.h"

#include "Canvas.h"
#include "Graphics.h"
#include "TextCodex.h"

#include <algorithm>

namespace Ui
{
	namespace
	{
		/** @brief UI Canvas 像素尺寸上限，避免 ScratchImage 分配过大。 */
		constexpr unsigned kMaxCanvasPixelDim = 2048u;

		/**
		 * @brief 将像素尺寸限制在 [1, kMaxCanvasPixelDim]。
		 */
		[[nodiscard]] unsigned ClampCanvasPixelDim(const unsigned value) noexcept
		{
			return std::max(1u, std::min(value, kMaxCanvasPixelDim));
		}

		void DrawBoxBorder(::Canvas& c, const unsigned border, const Color borderColor)
		{
			if (border == 0u)
				return;

			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			if (border * 2u >= w || border * 2u >= h)
				return;

			auto fill = [&](const unsigned x0, const unsigned y0, const unsigned x1, const unsigned y1)
			{
				for (unsigned y = y0; y <= y1; ++y)
					for (unsigned x = x0; x <= x1; ++x)
						c.PutPixel(x, y, borderColor);
			};

			fill(0u, 0u, w - 1u, border - 1u);
			fill(0u, h - border, w - 1u, h - 1u);
			fill(0u, border, border - 1u, h - border - 1u);
			fill(w - border, border, w - 1u, h - border - 1u);
		}
	}

	DropdownListItemCanvasView::DropdownListItemCanvasView(
		Graphics& gfx,
		const unsigned pixelWidth,
		const unsigned pixelHeight,
		const DropdownCanvasStyle& style)
		:
		style_(style),
		bgCanvas_(std::make_unique<Canvas2D>(gfx, 1u, 1u)),
		textCanvas_(std::make_unique<Canvas2D>(
			gfx,
			ClampCanvasPixelDim(pixelWidth),
			ClampCanvasPixelDim(pixelHeight)))
	{
		textCanvas_->Clear(Colors::None);
	}

	void DropdownListItemCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		bgCanvas_->LinkTechniques(rg);
		textCanvas_->LinkTechniques(rg);
	}

	void DropdownListItemCanvasView::Submit(const std::size_t channelMask) const
	{
		bgCanvas_->Submit(channelMask);
		textCanvas_->Submit(channelMask);
	}

	Color DropdownListItemCanvasView::BackgroundForRow_(const DropdownListItemViewModel& vm) const noexcept
	{
		if (vm.highlighted)
			return style_.itemHighlight;
		if (vm.selected)
			return style_.itemSelected;
		return style_.itemNormal;
	}

	void DropdownListItemCanvasView::ApplyLayout(
		const float centerX,
		const float centerY,
		const float width,
		const float height) noexcept
	{
		const float safeW = std::max(1.0f, width);
		const float safeH = std::max(1.0f, height);
		const DirectX::XMFLOAT3 pos{ centerX, centerY, 0.0f };
		const DirectX::XMFLOAT3 scale{ safeW, safeH, 1.0f };
		bgCanvas_->SetPosition(pos);
		bgCanvas_->SetScale(scale);
		textCanvas_->SetPosition(pos);
		textCanvas_->SetScale(scale);
	}

	void DropdownListItemCanvasView::RepaintBackground_(const DropdownListItemViewModel& vm)
	{
		::Canvas& c = *bgCanvas_;
		c.Clear(BackgroundForRow_(vm));
	}

	void DropdownListItemCanvasView::RepaintText_(const DropdownListItemViewModel& vm)
	{
		::Canvas& c = *textCanvas_;
		c.Clear(Colors::None);

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = vm.label.empty() ? " " : vm.label;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = style_.fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = style_.headerPaddingPx;
		rq.maxWidthPx = static_cast<float>(c.GetCanvasWidth());
		rq.defaultColor = style_.itemText;
		rq.backgroundColor = Colors::None;
		ctx.Render(c);
	}

	void DropdownListItemCanvasView::SyncFrom(const DropdownListItemViewModel& vm)
	{
		const bool bgDirty = !hasPainted_
			|| vm.highlighted != lastPainted_.highlighted
			|| vm.selected != lastPainted_.selected;

		const bool textDirty = !hasPainted_
			|| vm.label != lastPainted_.label;

		if (bgDirty)
			RepaintBackground_(vm);

		if (textDirty)
			RepaintText_(vm);

		if (bgDirty || textDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
