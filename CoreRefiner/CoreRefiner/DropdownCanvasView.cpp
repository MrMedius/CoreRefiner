#include "DropdownCanvasView.h"

#include "Canvas.h"
#include "Graphics.h"
#include "TextCodex.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	namespace
	{
		/** @brief UI Canvas 像素尺寸上限。 */
		constexpr unsigned kMaxCanvasPixelDim = 2048u;

		/** @brief 列表项数量上限。 */
		constexpr std::size_t kMaxListItemCount = 64u;

		/**
		 * @brief 将像素尺寸限制在 [1, kMaxCanvasPixelDim]。
		 */
		[[nodiscard]] unsigned ClampCanvasPixelDim(const unsigned value) noexcept
		{
			return std::max(1u, std::min(value, kMaxCanvasPixelDim));
		}

		void FillRect(::Canvas& c, unsigned x0, unsigned y0, unsigned x1, unsigned y1, Color col)
		{
			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			if (w == 0u || h == 0u)
				return;

			const unsigned left = std::min(x0, x1);
			const unsigned right = std::min(std::max(x0, x1), w - 1u);
			const unsigned top = std::min(y0, y1);
			const unsigned bottom = std::min(std::max(y0, y1), h - 1u);

			for (unsigned y = top; y <= bottom; ++y)
				for (unsigned x = left; x <= right; ++x)
					c.PutPixel(x, y, col);
		}

		void DrawBoxBorder(::Canvas& c, const unsigned border, const Color borderColor)
		{
			if (border == 0u)
				return;

			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			if (border * 2u >= w || border * 2u >= h)
				return;

			FillRect(c, 0u, 0u, w - 1u, border - 1u, borderColor);
			FillRect(c, 0u, h - border, w - 1u, h - 1u, borderColor);
			FillRect(c, 0u, border, border - 1u, h - border - 1u, borderColor);
			FillRect(c, w - border, border, w - 1u, h - border - 1u, borderColor);
		}

		[[nodiscard]] bool IsDisabledPhase(const UiVisualPhase phase) noexcept
		{
			return phase == UiVisualPhase::Disabled;
		}

		void TintWhiteShapePixels(::Canvas& c, const Color color)
		{
			const unsigned w = c.GetCanvasWidth();
			const unsigned h = c.GetCanvasHeight();
			for (unsigned y = 0u; y < h; ++y)
			{
				for (unsigned x = 0u; x < w; ++x)
				{
					const Color px = c.GetPixel(x, y);
					if (px.GetA() > 0u)
						c.PutPixel(x, y, color);
				}
			}
		}
	}

	DropdownCanvasView::DropdownCanvasView(
		Graphics& gfx,
		const unsigned headerPixelWidth,
		const unsigned headerPixelHeight,
		DropdownCanvasStyle style)
		:
		style_(std::move(style)),
		gfx_(gfx),
		headerPixelWidth_(ClampCanvasPixelDim(headerPixelWidth)),
		headerPixelHeight_(ClampCanvasPixelDim(headerPixelHeight)),
		headerBgCanvas_(std::make_unique<Canvas2D>(gfx, headerPixelWidth_, headerPixelHeight_)),
		headerTextCanvas_(std::make_unique<Canvas2D>(gfx, headerPixelWidth_, headerPixelHeight_)),
		listPanelBgCanvas_(std::make_unique<Canvas2D>(gfx, 1u, 1u)),
		scrollbarTrackCanvas_(std::make_unique<Canvas2D>(gfx, 1u, 1u)),
		scrollbarThumbCanvas_(std::make_unique<Canvas2D>(gfx, 1u, 1u)),
		arrowCanvas_(std::make_unique<Canvas2D>(
			gfx,
			std::max(8u, style_.arrowWidthPx),
			std::max(8u, style_.arrowWidthPx)))
	{
		headerTextCanvas_->Clear(Colors::None);
		arrowCanvas_->SetRotation(0.0f, 0.0f, -90.0f);
		scrollbarTrackCanvas_->Clear(style_.scrollbarTrack);
		scrollbarTrackPainted_ = true;

		BakeArrowGeometry_();
		SyncArrowOrientation_(false);
	}

	void DropdownCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		linkedRg_ = &rg;
		headerBgCanvas_->LinkTechniques(rg);
		headerTextCanvas_->LinkTechniques(rg);
		listPanelBgCanvas_->LinkTechniques(rg);
		scrollbarTrackCanvas_->LinkTechniques(rg);
		scrollbarThumbCanvas_->LinkTechniques(rg);
		arrowCanvas_->LinkTechniques(rg);
		for (const auto& item : listItems_)
			item->LinkTechniques(rg);
	}

	void DropdownCanvasView::Submit(const std::size_t channelMask) const
	{
		headerBgCanvas_->Submit(channelMask);
		headerTextCanvas_->Submit(channelMask);
		arrowCanvas_->Submit(channelMask);

		if (listVisible_)
		{
			listPanelBgCanvas_->Submit(channelMask);
			for (std::size_t i = 0; i < activeListItemCount_; ++i)
				listItems_[i]->Submit(channelMask);
		}

		if (scrollbarVisible_)
		{
			scrollbarTrackCanvas_->Submit(channelMask);
			scrollbarThumbCanvas_->Submit(channelMask);
		}
	}

	Color DropdownCanvasView::HeaderBackgroundForPhase(const UiVisualPhase phase) const noexcept
	{
		switch (phase)
		{
		case UiVisualPhase::Disabled: return style_.headerDisabled;
		case UiVisualPhase::Pressed:  return style_.headerPressed;
		case UiVisualPhase::Focused:  return style_.headerFocused;
		default:                      return style_.headerNormal;
		}
	}

	Color DropdownCanvasView::HeaderTextColorForPhase(const UiVisualPhase phase) const noexcept
	{
		return IsDisabledPhase(phase) ? style_.headerTextDisabled : style_.headerTextNormal;
	}

	void DropdownCanvasView::ApplyLayout(
		const float centerX,
		const float centerY,
		const float width,
		const float headerHeight) noexcept
	{
		layoutCenterX_ = centerX;
		layoutCenterY_ = centerY;
		layoutWidth_ = width;
		layoutHeight_ = headerHeight;

		const DirectX::XMFLOAT3 headerPos{ centerX, centerY, 0.0f };
		const DirectX::XMFLOAT3 headerScale{ width, headerHeight, 1.0f };
		headerBgCanvas_->SetPosition(headerPos);
		headerBgCanvas_->SetScale(headerScale);
		headerTextCanvas_->SetPosition(headerPos);
		headerTextCanvas_->SetScale(headerScale);

		ApplyArrowLayout_();
	}

	void DropdownCanvasView::ApplyArrowLayout_() noexcept
	{
		const float arrowLogical = std::min(
			static_cast<float>(style_.arrowWidthPx),
			layoutHeight_ * 0.55f);
		const float halfHeaderW = layoutWidth_ * 0.5f;
		const float halfArrow = arrowLogical * 0.5f;
		const float arrowCenterX = layoutCenterX_ + halfHeaderW
			- static_cast<float>(style_.headerPaddingPx) - halfArrow;

		const DirectX::XMFLOAT3 arrowPos{ arrowCenterX, layoutCenterY_, 0.0f };
		const DirectX::XMFLOAT3 arrowScale{ arrowLogical, arrowLogical, 1.0f };
		arrowCanvas_->SetPosition(arrowPos);
		arrowCanvas_->SetScale(arrowScale);
	}

	void DropdownCanvasView::RepaintListPanelBackground_()
	{
		::Canvas& c = *listPanelBgCanvas_;
		c.Clear(style_.itemNormal);
		listPanelPainted_ = true;
	}

	void DropdownCanvasView::EnsureListItemCount_(const std::size_t count)
	{
		const std::size_t cappedCount = std::min(count, kMaxListItemCount);

		while (listItems_.size() < cappedCount)
		{
			auto item = std::make_unique<DropdownListItemCanvasView>(
				gfx_,
				headerPixelWidth_,
				headerPixelHeight_,
				style_);
			if (linkedRg_ != nullptr)
				item->LinkTechniques(*linkedRg_);
			listItems_.push_back(std::move(item));
		}
	}

	void DropdownCanvasView::ApplyListLayout_(const DropdownViewModel& vm)
	{
		if (!vm.expanded || vm.optionLabels.empty() || vm.visibleItemCount <= 0)
		{
			listVisible_ = false;
			activeListItemCount_ = 0u;
			scrollbarVisible_ = false;
			return;
		}

		listVisible_ = true;
		activeListItemCount_ = static_cast<std::size_t>(vm.visibleItemCount);

		const float listLogicalH = vm.listViewportHeight;
		const float headerBottom = layoutCenterY_ + layoutHeight_ * 0.5f + vm.listOffsetY;
		const float listCenterY = headerBottom + listLogicalH * 0.5f;

		const DirectX::XMFLOAT3 panelPos{ layoutCenterX_, listCenterY, 0.0f };
		const DirectX::XMFLOAT3 panelScale{ layoutWidth_, listLogicalH, 1.0f };
		listPanelBgCanvas_->SetPosition(panelPos);
		listPanelBgCanvas_->SetScale(panelScale);

		const float itemH = vm.itemHeight;
		for (int i = 0; i < vm.visibleItemCount; ++i)
		{
			const float itemCenterY = headerBottom + itemH * (static_cast<float>(i) + 0.5f);
			listItems_[static_cast<size_t>(i)]->ApplyLayout(layoutCenterX_, itemCenterY, layoutWidth_, itemH);
		}

		ApplyScrollbarLayout_(vm);
	}

	void DropdownCanvasView::ApplyScrollbarLayout_(const DropdownViewModel& vm)
	{
		if (!vm.showScrollbar || vm.listViewportHeight <= 0.0f)
		{
			scrollbarVisible_ = false;
			return;
		}

		scrollbarVisible_ = true;

		const float headerBottom = layoutCenterY_ + layoutHeight_ * 0.5f + vm.listOffsetY;
		const float halfHeaderW = layoutWidth_ * 0.5f;
		const float trackCenterX = layoutCenterX_ + halfHeaderW - vm.scrollbarWidth * 0.5f;
		const float trackCenterY = headerBottom + vm.listViewportHeight * 0.5f;

		const DirectX::XMFLOAT3 trackPos{ trackCenterX, trackCenterY, 0.0f };
		const DirectX::XMFLOAT3 trackScale{ vm.scrollbarWidth, vm.listViewportHeight, 1.0f };
		scrollbarTrackCanvas_->SetPosition(trackPos);
		scrollbarTrackCanvas_->SetScale(trackScale);

		const float thumbH = std::max(
			vm.itemHeight * 0.35f,
			vm.listViewportHeight * vm.scrollThumbNormalizedSize);
		const float movable = std::max(0.0f, vm.listViewportHeight - thumbH);
		const float thumbTop = headerBottom + movable * vm.scrollThumbNormalizedPos;
		const float thumbCenterY = thumbTop + thumbH * 0.5f;

		const unsigned thumbPixelW = ClampCanvasPixelDim(
			static_cast<unsigned>(std::max(1.0f, std::round(vm.scrollbarWidth))));
		const unsigned thumbPixelH = ClampCanvasPixelDim(
			static_cast<unsigned>(std::max(1.0f, std::round(thumbH))));
		if (thumbPixelW != scrollbarThumbPixelWidth_ || thumbPixelH != scrollbarThumbPixelHeight_)
		{
			scrollbarThumbPixelWidth_ = thumbPixelW;
			scrollbarThumbPixelHeight_ = thumbPixelH;
			scrollbarThumbCanvas_->Resize(thumbPixelW, thumbPixelH);
			scrollbarThumbPainted_ = false;
		}

		const DirectX::XMFLOAT3 thumbPos{ trackCenterX, thumbCenterY, 0.0f };
		const DirectX::XMFLOAT3 thumbScale{ vm.scrollbarWidth, thumbH, 1.0f };
		scrollbarThumbCanvas_->SetPosition(thumbPos);
		scrollbarThumbCanvas_->SetScale(thumbScale);
	}

	void DropdownCanvasView::RepaintScrollbarTrack_()
	{
		::Canvas& c = *scrollbarTrackCanvas_;
		c.Clear(style_.scrollbarTrack);
		scrollbarTrackPainted_ = true;
	}

	void DropdownCanvasView::RepaintScrollbarThumb_(const bool hovered)
	{
		::Canvas& c = *scrollbarThumbCanvas_;
		c.ApplyForm(Canvas::Form::RoundedRectangle, 0.5f);
		TintWhiteShapePixels(c, hovered ? style_.scrollbarThumbHover : style_.scrollbarThumb);
		scrollbarThumbPainted_ = true;
		scrollbarThumbHovered_ = hovered;
	}

	void DropdownCanvasView::SyncListItems_(const DropdownViewModel& vm)
	{
		if (!vm.expanded || vm.optionLabels.empty() || vm.visibleItemCount <= 0)
		{
			listVisible_ = false;
			activeListItemCount_ = 0u;
			scrollbarVisible_ = false;
			return;
		}

		EnsureListItemCount_(static_cast<std::size_t>(vm.visibleItemCount));
		ApplyListLayout_(vm);

		for (int i = 0; i < vm.visibleItemCount; ++i)
		{
			const int optionIndex = vm.scrollOffset + i;
			if (optionIndex < 0 || optionIndex >= static_cast<int>(vm.optionLabels.size()))
				continue;

			const DropdownListItemViewModel rowVm{
				.label = vm.optionLabels[static_cast<size_t>(optionIndex)],
				.highlighted = optionIndex == vm.highlightIndex,
				.selected = optionIndex == vm.selectedIndex
			};
			listItems_[static_cast<size_t>(i)]->SyncFrom(rowVm);
		}
	}

	void DropdownCanvasView::RepaintHeaderBackground_(const UiVisualPhase phase)
	{
		::Canvas& c = *headerBgCanvas_;
		c.Clear(HeaderBackgroundForPhase(phase));
		DrawBoxBorder(c, style_.headerBorderPx, style_.headerBorder);
	}

	void DropdownCanvasView::BakeArrowGeometry_()
	{
		::Canvas& c = *arrowCanvas_;
		c.ApplyForm(Canvas::Form::Polygon, 3.0f);
		TintWhiteShapePixels(c, style_.arrowColor);
	}

	void DropdownCanvasView::SyncArrowOrientation_(const bool expanded) noexcept
	{
		if (arrowExpanded_ == expanded)
			return;

		arrowExpanded_ = expanded;
		// 2D UI 平面内旋转用 yaw（Z 轴）；与 SliderCanvasView 一致。
		const float yawDeg = expanded ? 180.0f : -90.0f;
		arrowCanvas_->SetRotation(0.0f, 0.0f, yawDeg);
	}

	void DropdownCanvasView::RepaintHeaderText_(const DropdownViewModel& vm)
	{
		::Canvas& c = *headerTextCanvas_;
		const unsigned w = c.GetCanvasWidth();
		const unsigned reservedArrow = style_.arrowWidthPx + style_.headerPaddingPx;
		const unsigned textMaxW = (w > reservedArrow + style_.headerPaddingPx)
			? w - reservedArrow - style_.headerPaddingPx
			: w;

		c.Clear(Colors::None);

		auto ctx = TextCodex::Get().BeginDraw();
		Text::RenderRequest& rq = ctx.Request();
		rq.text = vm.selectedLabel.empty() ? " " : vm.selectedLabel;
		rq.canvasMode = Text::CanvasMode::Fixed;
		rq.clearMode = Text::ClearMode::NoClear;
		rq.primaryFont = style_.primaryFont;
		rq.style.fontSize = style_.fontSize;
		rq.style.paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		rq.style.textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
		rq.style.wordWrapEnabled = false;
		rq.paddingPx = style_.headerPaddingPx;
		rq.maxWidthPx = static_cast<float>(textMaxW);
		rq.defaultColor = HeaderTextColorForPhase(vm.headerPhase);
		rq.backgroundColor = Colors::None;
		ctx.Render(c);
	}

	void DropdownCanvasView::SyncFrom(const DropdownViewModel& vm)
	{
		const bool bgDirty = !hasPainted_
			|| vm.headerPhase != lastPainted_.headerPhase;

		const bool textDirty = !hasPainted_
			|| vm.selectedLabel != lastPainted_.selectedLabel
			|| IsDisabledPhase(vm.headerPhase) != IsDisabledPhase(lastPainted_.headerPhase);

		const bool arrowDirty = !hasPainted_
			|| vm.expanded != lastPainted_.expanded;

		const bool listStructureDirty = !hasPainted_
			|| vm.expanded != lastPainted_.expanded
			|| vm.optionLabels != lastPainted_.optionLabels
			|| vm.itemHeight != lastPainted_.itemHeight
			|| vm.listOffsetY != lastPainted_.listOffsetY
			|| vm.scrollOffset != lastPainted_.scrollOffset
			|| vm.visibleItemCount != lastPainted_.visibleItemCount
			|| vm.showScrollbar != lastPainted_.showScrollbar
			|| vm.listViewportHeight != lastPainted_.listViewportHeight
			|| vm.scrollbarWidth != lastPainted_.scrollbarWidth
			|| vm.scrollThumbNormalizedPos != lastPainted_.scrollThumbNormalizedPos
			|| vm.scrollThumbNormalizedSize != lastPainted_.scrollThumbNormalizedSize;

		const bool listContentDirty = !hasPainted_
			|| vm.expanded != lastPainted_.expanded
			|| vm.optionLabels != lastPainted_.optionLabels
			|| vm.highlightIndex != lastPainted_.highlightIndex
			|| vm.selectedIndex != lastPainted_.selectedIndex
			|| vm.scrollOffset != lastPainted_.scrollOffset
			|| vm.visibleItemCount != lastPainted_.visibleItemCount;

		const bool scrollbarLayoutDirty = !hasPainted_
			|| vm.showScrollbar != lastPainted_.showScrollbar
			|| vm.scrollThumbNormalizedPos != lastPainted_.scrollThumbNormalizedPos
			|| vm.scrollThumbNormalizedSize != lastPainted_.scrollThumbNormalizedSize
			|| vm.listViewportHeight != lastPainted_.listViewportHeight
			|| vm.scrollbarWidth != lastPainted_.scrollbarWidth
			|| vm.listOffsetY != lastPainted_.listOffsetY;

		const bool scrollbarPaintDirty = !hasPainted_
			|| vm.showScrollbar != lastPainted_.showScrollbar
			|| vm.scrollbarHovered != lastPainted_.scrollbarHovered
			|| vm.scrollThumbNormalizedSize != lastPainted_.scrollThumbNormalizedSize
			|| vm.listViewportHeight != lastPainted_.listViewportHeight
			|| vm.scrollbarWidth != lastPainted_.scrollbarWidth;

		if (bgDirty)
			RepaintHeaderBackground_(vm.headerPhase);

		if (textDirty)
			RepaintHeaderText_(vm);

		if (arrowDirty)
			SyncArrowOrientation_(vm.expanded);

		if (vm.expanded && !listPanelPainted_)
			RepaintListPanelBackground_();

		if (listContentDirty)
			SyncListItems_(vm);
		else if (listStructureDirty)
			ApplyListLayout_(vm);
		else if (scrollbarLayoutDirty)
			ApplyScrollbarLayout_(vm);

		if (vm.showScrollbar && (!scrollbarThumbPainted_ || scrollbarPaintDirty))
			RepaintScrollbarThumb_(vm.scrollbarHovered);

		if (bgDirty || textDirty || arrowDirty || listStructureDirty || listContentDirty
			|| scrollbarLayoutDirty || scrollbarPaintDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
