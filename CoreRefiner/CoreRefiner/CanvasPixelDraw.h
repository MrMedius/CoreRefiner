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
}
