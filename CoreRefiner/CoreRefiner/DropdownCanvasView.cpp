#include "DropdownCanvasView.h"

#include "Canvas.h"
#include "CanvasPixelDraw.h"
#include "Graphics.h"
#include "TextCodex.h"

#include <algorithm>
#include <cmath>

namespace Ui
{
	namespace
	{
		constexpr unsigned kMaxCanvasPixelDim = 2048u;

		constexpr std::size_t kMaxListItemCount = 64u;

		[[nodiscard]] unsigned ClampCanvasPixelDim(const unsigned value) noexcept
		{
			return std::max(1u, std::min(value, kMaxCanvasPixelDim));
		}

		[[nodiscard]] bool IsDisabledPhase(const UiVisualPhase phase) noexcept
		{
			return phase == UiVisualPhase::Disabled;
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
		arrowCanvas_(std::make_unique<Canvas2D>(
			gfx,
			std::max(8u, style_.arrowWidthPx),
			std::max(8u, style_.arrowWidthPx)))
	{
		headerTextCanvas_->Clear(Colors::None);
		arrowCanvas_->SetRotation(0.0f, 0.0f, -90.0f);

		BakeArrowGeometry_();
		SyncArrowOrientation_(false);
	}

	void DropdownCanvasView::LinkTechniques(Rgph::RenderGraph& rg)
	{
		linkedRg_ = &rg;
		headerBgCanvas_->LinkTechniques(rg);
		headerTextCanvas_->LinkTechniques(rg);
		listPanelBgCanvas_->LinkTechniques(rg);
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
		if (!vm.expanded || vm.optionLabels.empty())
		{
			listVisible_ = false;
			activeListItemCount_ = 0u;
			return;
		}

		listVisible_ = true;
		activeListItemCount_ = std::min(vm.optionLabels.size(), kMaxListItemCount);

		const float listLogicalH = vm.listHeight;
		const float headerBottom = layoutCenterY_ + layoutHeight_ * 0.5f + vm.listOffsetY;
		const float listCenterY = headerBottom + listLogicalH * 0.5f;

		const DirectX::XMFLOAT3 panelPos{ layoutCenterX_, listCenterY, 0.0f };
		const DirectX::XMFLOAT3 panelScale{ layoutWidth_, listLogicalH, 1.0f };
		listPanelBgCanvas_->SetPosition(panelPos);
		listPanelBgCanvas_->SetScale(panelScale);

		const float itemH = vm.itemHeight;
		for (std::size_t i = 0; i < activeListItemCount_; ++i)
		{
			const float itemCenterY = headerBottom + itemH * (static_cast<float>(i) + 0.5f);
			listItems_[i]->ApplyLayout(layoutCenterX_, itemCenterY, layoutWidth_, itemH);
		}
	}

	void DropdownCanvasView::SyncListItems_(const DropdownViewModel& vm)
	{
		if (!vm.expanded || vm.optionLabels.empty())
		{
			listVisible_ = false;
			activeListItemCount_ = 0u;
			return;
		}

		EnsureListItemCount_(vm.optionLabels.size());
		ApplyListLayout_(vm);

		for (std::size_t i = 0; i < activeListItemCount_; ++i)
		{
			const UiVisualPhase phase = i < vm.listItemPhases.size()
				? vm.listItemPhases[i]
				: UiVisualPhase::Normal;
			const DropdownListItemViewModel rowVm{
				.label = vm.optionLabels[i],
				.phase = phase,
				.highlighted = static_cast<int>(i) == vm.highlightIndex,
				.selected = static_cast<int>(i) == vm.selectedIndex
			};
			listItems_[i]->SyncFrom(rowVm);
		}
	}

	void DropdownCanvasView::RepaintHeaderBackground_(const UiVisualPhase phase)
	{
		::Canvas& c = *headerBgCanvas_;
		c.Clear(HeaderBackgroundForPhase(phase));
		const unsigned w = c.GetCanvasWidth();
		const unsigned h = c.GetCanvasHeight();
		if (w > 0u && h > 0u)
		{
			CanvasPixelDraw::DrawRectBorder(c, 0u, 0u, w - 1u, h - 1u, style_.headerBorderPx, style_.headerBorder);
		}
	}

	void DropdownCanvasView::BakeArrowGeometry_()
	{
		::Canvas& c = *arrowCanvas_;
		c.ApplyForm(Canvas::Form::Triangle, 0.0f);
		CanvasPixelDraw::TintOpaquePixels(c, style_.arrowColor);
	}

	void DropdownCanvasView::SyncArrowOrientation_(const bool expanded) noexcept
	{
		if (arrowExpanded_ == expanded)
			return;

		arrowExpanded_ = expanded;

		// Rotate the small triangle on right
		const float yawDeg = expanded ? 180.0f : -90.0f;
		arrowCanvas_->SetRotation(0.0f, 0.0f, yawDeg);
	}

	void DropdownCanvasView::RepaintHeaderText_(const DropdownViewModel& vm)
	{
		::Canvas& c = *headerTextCanvas_;
		const unsigned w = c.GetCanvasWidth();
		const unsigned pad = static_cast<unsigned>(style_.headerPaddingPx);
		const unsigned reservedArrow = style_.arrowWidthPx + style_.headerPaddingPx;
		const unsigned textMaxW = w > reservedArrow + style_.headerPaddingPx
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
		// dest 收成箭头左侧文字区。Fixed 不再认 maxWidthPx，避免字画进箭头。
		rq.SetDestRect(0.0f, 0.0f, static_cast<float>(textMaxW), static_cast<float>(c.GetCanvasHeight()));
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

		const bool arrowDirty = !hasPainted_ || vm.expanded != lastPainted_.expanded;

		const bool listStructureDirty = !hasPainted_
			|| vm.expanded != lastPainted_.expanded
			|| vm.optionLabels != lastPainted_.optionLabels
			|| vm.itemHeight != lastPainted_.itemHeight
			|| vm.listOffsetY != lastPainted_.listOffsetY
			|| vm.listHeight != lastPainted_.listHeight;

		const bool listContentDirty = !hasPainted_
			|| vm.expanded != lastPainted_.expanded
			|| vm.optionLabels != lastPainted_.optionLabels
			|| vm.highlightIndex != lastPainted_.highlightIndex
			|| vm.selectedIndex != lastPainted_.selectedIndex
			|| vm.listItemPhases != lastPainted_.listItemPhases;

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

		if (bgDirty || textDirty || arrowDirty || listStructureDirty || listContentDirty)
		{
			lastPainted_ = vm;
			hasPainted_ = true;
		}
	}
}
