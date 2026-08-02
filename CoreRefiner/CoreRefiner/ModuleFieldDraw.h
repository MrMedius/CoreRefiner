#pragma once

#include "Canvas2D.h"
#include "Colors.h"

#include <cmath>

namespace ModuleFieldDraw
{
	inline void DrawDisk(Canvas2D& canvas, int cx, int cy, int radius, Color color)
	{
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		const int r2 = radius * radius;
		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y > r2)
				{
					continue;
				}
				const int px = cx + x;
				const int py = cy + y;
				if (px < 0 || py < 0 || px >= w || py >= h)
				{
					continue;
				}
				canvas.PutPixel(static_cast<unsigned>(px), static_cast<unsigned>(py), color);
			}
		}
	}

	inline void DrawRing(Canvas2D& canvas, int cx, int cy, int radius, int thickness, Color color)
	{
		if (radius <= 0 || thickness <= 0)
		{
			return;
		}
		const int w = static_cast<int>(canvas.GetCanvasWidth());
		const int h = static_cast<int>(canvas.GetCanvasHeight());
		const int outer = radius + thickness / 2;
		const int inner = (radius > thickness / 2) ? (radius - (thickness + 1) / 2) : 0;
		const int outer2 = outer * outer;
		const int inner2 = inner * inner;
		for (int y = -outer; y <= outer; ++y)
		{
			for (int x = -outer; x <= outer; ++x)
			{
				const int d2 = x * x + y * y;
				if (d2 > outer2 || d2 < inner2)
				{
					continue;
				}
				const int px = cx + x;
				const int py = cy + y;
				if (px < 0 || py < 0 || px >= w || py >= h)
				{
					continue;
				}
				canvas.PutPixel(static_cast<unsigned>(px), static_cast<unsigned>(py), color);
			}
		}
	}

	/** @brief Field local (0,0)=center → canvas pixel. */
	inline void LocalToPixel(
		float localX,
		float localY,
		unsigned canvasW,
		unsigned canvasH,
		int& outPx,
		int& outPy) noexcept
	{
		outPx = static_cast<int>(std::lround(static_cast<float>(canvasW) * 0.5f + localX));
		outPy = static_cast<int>(std::lround(static_cast<float>(canvasH) * 0.5f + localY));
	}
}
