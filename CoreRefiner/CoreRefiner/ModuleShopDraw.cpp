#include "ModuleShopDraw.h"

namespace ModuleShopDraw
{
	// ---- 炼成：三格和右侧四键 ----

	void FillRefineLayout(int cw, int ch, RefineLayout& layout) noexcept
	{
		// pad 是内边距。innerW、innerH 是扣掉两边内边距后的内容宽高。
		layout.pad = (std::max)(kRefinePadMin, ch / kRefinePadDiv);
		const int innerW = (std::max)(1, cw - layout.pad * 2);
		const int innerH = (std::max)(1, ch - layout.pad * 2);
		layout.btnGap = kRefineButtonGap;
		layout.btnW = kRefineButtonW;
		layout.btnH = (std::max)(kRefineButtonHMin, (innerH - layout.btnGap * (kRefineButtonCount - 1)) / kRefineButtonCount);

		// side 是格边，先占满内容高度。regionGap 是格与格、格与按钮的空隙。
		int side = innerH;
		int regionGap = kRefineRegionGapWanted;
		// 一整排的宽度：三格、三段空隙、一列按钮。
		auto clusterW = [&]() noexcept
		{
			return side * kRefineSlotCount + regionGap * kRefineSlotCount + layout.btnW;
		};
		if (clusterW() > innerW)
		{
			regionGap = (std::max)(kRefineRegionGapMin, (innerW - side * kRefineSlotCount - layout.btnW) / kRefineSlotCount);
		}
		if (clusterW() > innerW)
		{
			side = (std::max)(kRefineSideMin, (innerW - regionGap * kRefineSlotCount - layout.btnW) / kRefineSlotCount);
		}
		if (clusterW() > innerW)
		{
			layout.btnW = (std::max)(kRefineButtonWMin, innerW - side * kRefineSlotCount - regionGap * kRefineSlotCount);
		}

		layout.side = side;
		layout.slotTop = layout.pad + (std::max)(0, innerH - side) / 2;
		// groupX 是这一排在内容区里居中后的左边。
		const int groupX = layout.pad + (std::max)(0, innerW - clusterW()) / 2;
		layout.slotXs[0] = groupX;
		layout.slotXs[1] = groupX + side + regionGap;
		layout.slotXs[2] = groupX + (side + regionGap) * 2;
		layout.btnX = groupX + (side + regionGap) * kRefineSlotCount;
		// btnColH 是四键加间距的总高度，用来把整列在内容区里垂直居中。
		const int btnColH = layout.btnH * kRefineButtonCount + layout.btnGap * (kRefineButtonCount - 1);
		layout.btnTop = layout.pad + (std::max)(0, innerH - btnColH) / 2;
	}

	// ---- 奥义：栏位和按钮 ----

	// 内部栏边长。奥义栏边长乘 3/4，至少 1 像素。
	int UltraInnerSide_(int ultraSide) noexcept
	{
		return (std::max)(1, ultraSide * kUltraInnerNumer / kUltraInnerDenom);
	}

	// 右侧按钮左边能放下几格，不按容量截断。多出来的格子靠滚动看。
	int FitUltraInnerSlots_(const UltraLayout& layout) noexcept
	{
		if (layout.innerSide <= 0)
		{
			return 0;
		}
		// limit 是内部栏不能越过的右边。stride 是相邻两格左边的距离。
		const int limit = layout.btnX - layout.innerGap;
		const int stride = layout.innerSide + layout.innerGap;
		if (stride <= 0 || layout.innerX0 > limit)
		{
			return 0;
		}
		int count = 0;
		int x = layout.innerX0;
		while (x + layout.innerSide <= limit)
		{
			++count;
			x += stride;
		}
		return count;
	}

	void FillUltraLayout(int cw, int ch, int capacity, UltraLayout& layout) noexcept
	{
		layout.capacity = capacity;
		// pad 是内边距。innerW、innerH 是扣掉两边内边距后的内容宽高。
		layout.pad = (std::max)(kUltraPadMin, ch / kUltraPadDiv);
		const int innerW = (std::max)(1, cw - layout.pad * 2);
		const int innerH = (std::max)(1, ch - layout.pad * 2);

		layout.btnGap = kUltraButtonGap;
		layout.btnW = (std::min)(kUltraButtonWMax, (std::max)(kUltraButtonWMin, innerW / kUltraButtonWDiv));
		layout.btnH = (std::max)(kUltraButtonHMin, (innerH - layout.btnGap * (kUltraButtonCount - 1)) / kUltraButtonCount);
		layout.innerGap = (std::max)(kUltraInnerGapMin, layout.pad / 2);

		// ultraSide、innerSide 先占满内容高度。有容量时，下面要留出翻页条。
		int ultraSide = innerH;
		int innerSide = UltraInnerSide_(ultraSide);
		if (capacity > 0 && innerH - innerSide < kUltraPagerMinH)
		{
			const int innerMax = (std::max)(1, innerH - kUltraPagerMinH);
			ultraSide = innerMax * kUltraInnerDenom / kUltraInnerNumer;
			ultraSide = (std::min)((std::max)(kUltraSideFloor, ultraSide), innerH);
			innerSide = UltraInnerSide_(ultraSide);
			while (ultraSide > kUltraSideFloor && innerH - innerSide < kUltraPagerMinH)
			{
				--ultraSide;
				innerSide = UltraInnerSide_(ultraSide);
			}
		}
		// 窄到连一格都放不下时，奥义栏和内部栏按 4:3 一起缩小。
		if (capacity > 0)
		{
			// room 是右侧按钮左边、扣掉两段空隙后剩下的宽度。
			const int room = innerW - layout.btnW - layout.innerGap * 2;
			if (ultraSide + innerSide > room && room > kUltraRoomMin)
			{
				const int fitted = (std::min)(
					ultraSide,
					(std::max)(kUltraFittedMin, room * kUltraInnerDenom / (kUltraInnerDenom + kUltraInnerNumer)));
				ultraSide = (std::min)(fitted, innerH);
				innerSide = UltraInnerSide_(ultraSide);
			}
		}

		layout.ultraSide = (std::max)(1, (std::min)(ultraSide, innerH));
		layout.innerSide = UltraInnerSide_(layout.ultraSide);
		layout.ultraX = layout.pad;
		layout.ultraY = layout.pad;
		layout.btnX = cw - layout.pad - layout.btnW;
		// maxUltra 是奥义栏在按钮左边最多能用的宽度。
		const int maxUltra = layout.btnX - layout.innerGap - layout.ultraX;
		if (maxUltra > 0 && layout.ultraSide > maxUltra)
		{
			layout.ultraSide = maxUltra;
			layout.innerSide = UltraInnerSide_(layout.ultraSide);
		}
		layout.innerX0 = layout.ultraX + layout.ultraSide + layout.innerGap;
		layout.innerY = layout.ultraY;

		// colH 是三键加间距的总高度，用来把整列在内容区里垂直居中。
		const int colH = layout.btnH * kUltraButtonCount + layout.btnGap * (kUltraButtonCount - 1);
		layout.btnY = layout.pad + (std::max)(0, innerH - colH) / 2;
		layout.windowCount = FitUltraInnerSlots_(layout);

		layout.forgeW = (std::min)(kUltraForgeWMax, (std::max)(kUltraForgeWMin, cw / kUltraForgeWDiv));
		layout.forgeH = (std::min)(kUltraForgeHMax, (std::max)(kUltraForgeHMin, ch / kUltraForgeHDiv));
		layout.forgeW = (std::min)(layout.forgeW, (std::max)(1, cw - kUltraForgeMargin));
		layout.forgeH = (std::min)(layout.forgeH, (std::max)(1, ch - kUltraForgeMargin));
		layout.forgeX = (std::max)(0, (cw - layout.forgeW) / 2);
		layout.forgeY = (std::max)(0, (ch - layout.forgeH) / 2);

		// contentBottom 是内容区的底边。翻页条贴在内部栏下面，右边界让开按钮。
		const int contentBottom = layout.pad + innerH;
		layout.pagerX = layout.innerX0;
		layout.pagerY = layout.innerY + layout.innerSide;
		layout.pagerW = (std::max)(0, layout.btnX - layout.innerGap - layout.pagerX);
		layout.pagerH = (std::max)(0, contentBottom - layout.pagerY);
	}

	// ---- 奥义：翻页条 ----

	// 一个圆点的直径。半径不大于 0 时画成 1 像素。
	int PagerDotDiam_(int radius) noexcept
	{
		return (radius <= 0) ? 1 : (radius * 2 + 1);
	}

	// 一排圆点占用的宽度。count 是点数，gap 是点与点之间的空隙。
	int PagerDotsWidth_(int count, int radius, int gap) noexcept
	{
		if (count <= 0)
		{
			return 0;
		}
		return count * PagerDotDiam_(radius) + (std::max)(0, count - 1) * gap;
	}

	UltraPagerGlyphs BuildUltraPagerGlyphs(int pagerX, int pagerY, int pagerW, int pagerH, int capacity) noexcept
	{
		UltraPagerGlyphs glyphs{};
		if (capacity <= 0 || pagerW < kUltraPagerMinW || pagerH < kUltraPagerMinDrawH)
		{
			return glyphs;
		}
		// triH、triW 是三角的高和宽。两条三角并排放不下时不画。
		const int triH = (std::min)(kUltraPagerTriMax, (std::max)(1, pagerH - kUltraPagerTriInset));
		const int triW = (std::max)(kUltraPagerTriWMin, triH);
		if (triW * 2 >= pagerW)
		{
			return glyphs;
		}

		int radius = (std::min)(kUltraPagerDotRadiusMax, (std::max)(kUltraPagerDotRadiusMin, triH / kUltraPagerDotDiv));
		int gap = (std::max)(kUltraPagerDotGapMin, radius);
		// 左三角、空隙、圆点、空隙、右三角的总宽度。
		auto groupWidth = [&](int r, int g) noexcept
		{
			return triW + g + PagerDotsWidth_(capacity, r, g) + g + triW;
		};
		for (;;)
		{
			if (groupWidth(radius, gap) <= pagerW || (radius <= 0 && gap <= 0))
			{
				break;
			}
			if (gap > 1)
			{
				--gap;
			}
			else if (radius > 1)
			{
				--radius;
			}
			else if (gap > 0)
			{
				--gap;
			}
			else if (radius > 0)
			{
				--radius;
			}
			else
			{
				break;
			}
		}

		glyphs.valid = true;
		glyphs.count = capacity;
		glyphs.triW = triW;
		glyphs.triH = triH;
		glyphs.triY = pagerY + (std::max)(0, pagerH - triH) / 2;
		glyphs.cy = glyphs.triY + (triH - 1) / 2;
		glyphs.radius = radius;
		glyphs.gap = gap;

		const int used = groupWidth(radius, gap);
		if (used <= pagerW)
		{
			// origin 是整组居中后的左边。dotsLeft 是圆点区域的左边。
			const int origin = pagerX + (pagerW - used) / 2;
			glyphs.leftX = origin;
			const int dotsLeft = origin + triW + gap;
			glyphs.firstCx = dotsLeft + ((radius <= 0) ? 0 : radius);
			glyphs.stride = PagerDotDiam_(radius) + gap;
			glyphs.rightX = dotsLeft + PagerDotsWidth_(capacity, radius, gap) + gap;
			return glyphs;
		}

		// 缩到极限仍超宽时，三角留在条带内，点挤在两条三角中间。
		glyphs.leftX = pagerX;
		glyphs.rightX = pagerX + pagerW - triW;
		glyphs.radius = 0;
		glyphs.gap = 0;
		const int innerL = glyphs.leftX + triW;
		const int innerR = glyphs.rightX;
		if (capacity <= 1)
		{
			glyphs.firstCx = (innerL + innerR) / 2;
			glyphs.stride = 0;
		}
		else
		{
			const int span = (std::max)(0, innerR - innerL - 1);
			glyphs.stride = span / (capacity - 1);
			glyphs.firstCx = innerL;
		}
		return glyphs;
	}

	// 翻页三角。pointRight 为真时尖角朝右。lit 为真时用亮起的按钮色。
	void DrawPagerArrow_(Canvas2D& canvas, int x, int y, int w, int h, bool pointRight, bool lit)
	{
		if (w <= 1 || h <= 1)
		{
			return;
		}
		const Color fill = lit ? kBtnBgLit : kBtnBg;
		const Color frame = lit ? kBtnFrameLit : kBtnFrame;
		// midY 是尖角的竖直位置。baseX 是底边，tipX 是尖角。
		const int midY = y + (h - 1) / 2;
		const int baseX = pointRight ? x : (x + w - 1);
		const int tipX = pointRight ? (x + w - 1) : x;
		CanvasPixelDraw::FillTriangle(canvas, tipX, midY, baseX, y, baseX, y + h - 1, fill);
		CanvasPixelDraw::DrawTriangleOutline(canvas, tipX, midY, baseX, y, baseX, y + h - 1, frame);
	}

	// 翻页圆点。filled 为真时先填再描边，否则只描灭按钮的灰边。
	void DrawPagerDot_(Canvas2D& canvas, int cx, int cy, int radius, bool filled)
	{
		const Color fill = kBtnBgLit;
		const Color frame = filled ? kBtnFrameLit : kBtnFrame;
		if (radius <= 0)
		{
			CanvasPixelDraw::FillCircle(canvas, cx, cy, 0, filled ? fill : frame);
			return;
		}
		if (filled)
		{
			CanvasPixelDraw::FillCircle(canvas, cx, cy, radius, fill);
		}
		CanvasPixelDraw::DrawCircleOutline(canvas, cx, cy, radius, frame);
	}

	void DrawUltraPager(Canvas2D& canvas, const UltraLayout& layout, int visibleBegin, int visibleCount) noexcept
	{
		const UltraPagerGlyphs glyphs = BuildUltraPagerGlyphs(
			layout.pagerX,
			layout.pagerY,
			layout.pagerW,
			layout.pagerH,
			layout.capacity);
		if (!glyphs.valid)
		{
			return;
		}

		// canLeft、canRight 表示这一侧还能不能翻。到头的三角不亮。
		const bool canLeft = visibleBegin > 0;
		const bool canRight = visibleBegin + layout.windowCount < layout.capacity;
		DrawPagerArrow_(canvas, glyphs.leftX, glyphs.triY, glyphs.triW, glyphs.triH, false, canLeft);
		DrawPagerArrow_(canvas, glyphs.rightX, glyphs.triY, glyphs.triW, glyphs.triH, true, canRight);
		for (int i = 0; i < glyphs.count; ++i)
		{
			// onScreen 表示这个容量下标落在当前页里。落在页里的圆点填实。
			const bool onScreen = i >= visibleBegin && i < visibleBegin + visibleCount;
			DrawPagerDot_(canvas, glyphs.firstCx + i * glyphs.stride, glyphs.cy, glyphs.radius, onScreen);
		}
	}
}
