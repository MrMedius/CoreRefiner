#pragma once
#include "Canvas.h"
#include "Colors.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace CanvasPixelDraw
{
	/** @brief 画布内写像素；越界忽略。 */
	inline void PutPixelClamped(Canvas& canvas, int x, int y, Color c)
	{
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		if (x < 0 || y < 0 || x >= w || y >= h)
		{
			return;
		}
		canvas.PutPixel(static_cast<unsigned>(x), static_cast<unsigned>(y), c);
	}

	/** @brief 水平线段，端点会排序；越界像素忽略。 */
	inline void DrawHLine(Canvas& canvas, int x0, int x1, int y, Color c)
	{
		if (x1 < x0)
		{
			std::swap(x0, x1);
		}
		for (int x = x0; x <= x1; ++x)
		{
			PutPixelClamped(canvas, x, y, c);
		}
	}

	/** @brief 垂直线段，端点会排序；越界像素忽略。 */
	inline void DrawVLine(Canvas& canvas, int x, int y0, int y1, Color c)
	{
		if (y1 < y0)
		{
			std::swap(y0, y1);
		}
		for (int y = y0; y <= y1; ++y)
		{
			PutPixelClamped(canvas, x, y, c);
		}
	}

	/** @brief Bresenham 任意直线；越界像素忽略。 */
	inline void DrawLine(Canvas& canvas, int x0, int y0, int x1, int y1, Color c)
	{
		const int dx = std::abs(x1 - x0);
		const int dy = std::abs(y1 - y0);
		const int sx = (x0 < x1) ? 1 : -1;
		const int sy = (y0 < y1) ? 1 : -1;
		int err = dx - dy;
		for (;;)
		{
			PutPixelClamped(canvas, x0, y0, c);
			if (x0 == x1 && y0 == y1)
			{
				break;
			}
			const int e2 = 2 * err;
			if (e2 > -dy)
			{
				err -= dy;
				x0 += sx;
			}
			if (e2 < dx)
			{
				err += dx;
				y0 += sy;
			}
		}
	}

	/** @brief 轴对齐矩形描边。 */
	inline void DrawRectOutline(Canvas& canvas, int x0, int y0, int x1, int y1, Color c)
	{
		DrawHLine(canvas, x0, x1, y0, c);
		DrawHLine(canvas, x0, x1, y1, c);
		DrawVLine(canvas, x0, y0, y1, c);
		DrawVLine(canvas, x1, y0, y1, c);
	}

	// 像素是否在圆角矩形内（含边）。radius<=0 视为直角。
	[[nodiscard]] inline bool IsInsideRoundedRect(
		int x,
		int y,
		int left,
		int top,
		int right,
		int bottom,
		int radius) noexcept
	{
		if (x < left || x > right || y < top || y > bottom)
		{
			return false;
		}
		if (radius <= 0)
		{
			return true;
		}

		const int w = right - left + 1;
		const int h = bottom - top + 1;
		if (w <= 0 || h <= 0)
		{
			return false;
		}

		const int maxR = (std::min)((w - 1) / 2, (h - 1) / 2);
		const int r = (std::min)(radius, maxR);
		if (r <= 0)
		{
			return true;
		}

		const int innerLeft = left + r;
		const int innerRight = right - r;
		const int innerTop = top + r;
		const int innerBottom = bottom - r;
		if (x >= innerLeft && x <= innerRight)
		{
			return true;
		}
		if (y >= innerTop && y <= innerBottom)
		{
			return true;
		}

		const int cx = (x < innerLeft) ? innerLeft : innerRight;
		const int cy = (y < innerTop) ? innerTop : innerBottom;
		const int dx = x - cx;
		const int dy = y - cy;
		return dx * dx + dy * dy <= r * r;
	}

	// 轴对齐圆角实心矩形。
	inline void FillRoundedRect(Canvas& canvas, int x0, int y0, int x1, int y1, int radius, Color c)
	{
		int left = x0;
		int right = x1;
		int top = y0;
		int bottom = y1;
		if (right < left)
		{
			std::swap(left, right);
		}
		if (bottom < top)
		{
			std::swap(top, bottom);
		}

		for (int y = top; y <= bottom; ++y)
		{
			for (int x = left; x <= right; ++x)
			{
				if (IsInsideRoundedRect(x, y, left, top, right, bottom, radius))
				{
					PutPixelClamped(canvas, x, y, c);
				}
			}
		}
	}

	// 轴对齐圆角描边（1 像素，向内）。
	inline void DrawRoundedRectOutline(Canvas& canvas, int x0, int y0, int x1, int y1, int radius, Color c)
	{
		int left = x0;
		int right = x1;
		int top = y0;
		int bottom = y1;
		if (right < left)
		{
			std::swap(left, right);
		}
		if (bottom < top)
		{
			std::swap(top, bottom);
		}
		if (radius <= 0)
		{
			DrawRectOutline(canvas, left, top, right, bottom, c);
			return;
		}

		const bool innerOk = (right - left >= 2) && (bottom - top >= 2);
		const int innerLeft = left + 1;
		const int innerTop = top + 1;
		const int innerRight = right - 1;
		const int innerBottom = bottom - 1;
		const int innerRadius = radius - 1;
		for (int y = top; y <= bottom; ++y)
		{
			for (int x = left; x <= right; ++x)
			{
				if (!IsInsideRoundedRect(x, y, left, top, right, bottom, radius))
				{
					continue;
				}
				if (innerOk && IsInsideRoundedRect(
					x,
					y,
					innerLeft,
					innerTop,
					innerRight,
					innerBottom,
					innerRadius))
				{
					continue;
				}
				PutPixelClamped(canvas, x, y, c);
			}
		}
	}

	/**
	 * @brief 轴对齐矩形描边，thickness 圈向内加厚。
	 * @param thickness 线宽（像素）；小于 1 时不画。
	 */
	inline void DrawRectOutlineThick(Canvas& canvas, int x0, int y0, int x1, int y1, int thickness, Color c)
	{
		if (thickness < 1)
		{
			return;
		}
		for (int i = 0; i < thickness; ++i)
		{
			DrawRectOutline(canvas, x0 + i, y0 + i, x1 - i, y1 - i, c);
		}
	}

	/** @brief 轴对齐实心矩形；坐标钳到画布内。 */
	inline void FillRect(Canvas& canvas, unsigned x0, unsigned y0, unsigned x1, unsigned y1, Color c)
	{
		const unsigned w = canvas.GetCanvasWidth();
		const unsigned h = canvas.GetCanvasHeight();
		if (w == 0u || h == 0u)
		{
			return;
		}

		const unsigned left = std::min(x0, x1);
		const unsigned right = std::min(std::max(x0, x1), w - 1u);
		const unsigned top = std::min(y0, y1);
		const unsigned bottom = std::min(std::max(y0, y1), h - 1u);

		for (unsigned y = top; y <= bottom; ++y)
		{
			for (unsigned x = left; x <= right; ++x)
			{
				canvas.PutPixel(x, y, c);
			}
		}
	}

	/** @brief 轴对齐矩形边框（四边填充）。厚度为 0 或挤满内宽/内高时不画。 */
	inline void DrawRectBorder(
		Canvas& canvas,
		unsigned x0,
		unsigned y0,
		unsigned x1,
		unsigned y1,
		unsigned thickness,
		Color c)
	{
		if (thickness == 0u)
		{
			return;
		}

		const unsigned w = (x0 <= x1) ? (x1 - x0 + 1u) : (x0 - x1 + 1u);
		const unsigned h = (y0 <= y1) ? (y1 - y0 + 1u) : (y0 - y1 + 1u);
		if (thickness * 2u >= w || thickness * 2u >= h)
		{
			return;
		}

		const unsigned left = std::min(x0, x1);
		const unsigned right = std::max(x0, x1);
		const unsigned top = std::min(y0, y1);
		const unsigned bottom = std::max(y0, y1);

		FillRect(canvas, left, top, right, top + thickness - 1u, c);
		FillRect(canvas, left, bottom + 1u - thickness, right, bottom, c);
		FillRect(canvas, left, top + thickness, left + thickness - 1u, bottom - thickness, c);
		FillRect(canvas, right + 1u - thickness, top + thickness, right, bottom - thickness, c);
	}

	/** @brief 沿画布内壁绘制加厚矩形焦点环。 */
	inline void DrawFocusRing(Canvas& canvas, Color ring, unsigned thick)
	{
		const unsigned w = canvas.GetCanvasWidth();
		const unsigned h = canvas.GetCanvasHeight();
		if (w == 0u || h == 0u || thick == 0u)
		{
			return;
		}
		for (unsigned t = 0u; t < thick; ++t)
		{
			if (t >= w || t >= h)
			{
				break;
			}
			const unsigned y1 = t;
			const unsigned y2 = h - 1u - t;
			for (unsigned x = 0u; x < w; ++x)
			{
				canvas.PutPixel(x, y1, ring);
				if (y2 != y1)
				{
					canvas.PutPixel(x, y2, ring);
				}
			}
			const unsigned x1 = t;
			const unsigned x2 = w - 1u - t;
			for (unsigned y = y1; y <= y2; ++y)
			{
				canvas.PutPixel(x1, y, ring);
				if (x2 != x1)
				{
					canvas.PutPixel(x2, y, ring);
				}
			}
		}
	}

	/** @brief 将画布上所有不透明像素改成指定颜色；透明像素不动。 */
	inline void TintOpaquePixels(Canvas& canvas, Color color)
	{
		const unsigned w = canvas.GetCanvasWidth();
		const unsigned h = canvas.GetCanvasHeight();
		for (unsigned y = 0u; y < h; ++y)
		{
			for (unsigned x = 0u; x < w; ++x)
			{
				if (canvas.GetPixel(x, y).GetA() > 0u)
				{
					canvas.PutPixel(x, y, color);
				}
			}
		}
	}

	// 最近邻缩放拷贝到 dst。透明像素不写。src 空或目标边为 0 则跳过。
	inline void BlitNearest(
		Canvas& dst,
		const Canvas& src,
		int dx,
		int dy,
		unsigned destW,
		unsigned destH)
	{
		const unsigned sw = src.GetCanvasWidth();
		const unsigned sh = src.GetCanvasHeight();
		if (sw == 0u || sh == 0u || destW == 0u || destH == 0u)
		{
			return;
		}

		for (unsigned y = 0u; y < destH; ++y)
		{
			const unsigned sy = y * sh / destH;
			for (unsigned x = 0u; x < destW; ++x)
			{
				const unsigned sx = x * sw / destW;
				const Color c = src.GetPixel(sx, sy);
				if (c.GetA() == 0u)
				{
					continue;
				}
				PutPixelClamped(
					dst,
					dx + static_cast<int>(x),
					dy + static_cast<int>(y),
					c);
			}
		}
	}

	// 按不透明像素包围盒适配，居中拷到 dst 矩形。
	inline void BlitNearestCentered(
		Canvas& dst,
		const Canvas& src,
		int dx,
		int dy,
		unsigned destW,
		unsigned destH)
	{
		const unsigned sw = src.GetCanvasWidth();
		const unsigned sh = src.GetCanvasHeight();
		if (sw == 0u || sh == 0u || destW == 0u || destH == 0u)
		{
			return;
		}

		bool found = false;
		unsigned x0 = 0u;
		unsigned y0 = 0u;
		unsigned x1 = 0u;
		unsigned y1 = 0u;
		for (unsigned y = 0u; y < sh; ++y)
		{
			for (unsigned x = 0u; x < sw; ++x)
			{
				if (src.GetPixel(x, y).GetA() == 0u)
				{
					continue;
				}
				if (!found)
				{
					found = true;
					x0 = x1 = x;
					y0 = y1 = y;
				}
				else
				{
					if (x < x0) { x0 = x; }
					if (x > x1) { x1 = x; }
					if (y < y0) { y0 = y; }
					if (y > y1) { y1 = y; }
				}
			}
		}
		if (!found)
		{
			return;
		}

		const unsigned cw = x1 - x0 + 1u;
		const unsigned ch = y1 - y0 + 1u;
		unsigned fitW = destW;
		unsigned fitH = destH;
		if (cw * destH >= ch * destW)
		{
			fitW = destW;
			fitH = (std::max)(1u, ch * destW / cw);
		}
		else
		{
			fitH = destH;
			fitW = (std::max)(1u, cw * destH / ch);
		}

		const int ox = dx + static_cast<int>(destW - fitW) / 2;
		const int oy = dy + static_cast<int>(destH - fitH) / 2;
		for (unsigned y = 0u; y < fitH; ++y)
		{
			const unsigned sy = y0 + y * ch / fitH;
			for (unsigned x = 0u; x < fitW; ++x)
			{
				const unsigned sx = x0 + x * cw / fitW;
				const Color c = src.GetPixel(sx, sy);
				if (c.GetA() == 0u)
				{
					continue;
				}
				PutPixelClamped(
					dst,
					ox + static_cast<int>(x),
					oy + static_cast<int>(y),
					c);
			}
		}
	}
}
