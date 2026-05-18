#include "Canvas.h"
#include <algorithm>

namespace
{
	/**
	 * @brief 尖角朝上三角形（底边贴下边）。
	 */
	bool IsInsideTriangleApexUp(float px, float py, unsigned w, unsigned h) noexcept
	{
		if (w == 0u || h == 0u)
			return false;
		if (h == 1u)
			return true;
		const float x0 = (static_cast<float>(w) - 1.0f) * 0.5f;
		const float y0 = 0.0f;
		const float x1 = 0.0f;
		const float y1 = static_cast<float>(h - 1u);
		const float x2 = static_cast<float>(w - 1u);
		const float y2 = static_cast<float>(h - 1u);
		const float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
		if (std::fabs(denom) < 1e-6f)
			return false;
		const float u = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / denom;
		const float v = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / denom;
		const float wBary = 1.0f - u - v;
		return u >= 0.0f && v >= 0.0f && wBary >= 0.0f;
	}
	bool IsInsideEllipse(float px, float py, float cx, float cy, float rx, float ry) noexcept
	{
		const float dx = (px - cx) / rx;
		const float dy = (py - cy) / ry;
		return dx * dx + dy * dy <= 1.0f;
	}
	bool IsInsideCircle(float px, float py, float cx, float cy, float r) noexcept
	{
		const float dx = px - cx;
		const float dy = py - cy;
		return dx * dx + dy * dy <= r * r;
	}
	/**
	 * @brief 内接菱形：|x-cx|/rx + |y-cy|/ry <= 1。
	 */
	bool IsInsideDiamond(float px, float py, float cx, float cy, float rx, float ry) noexcept
	{
		return std::fabs(px - cx) / rx + std::fabs(py - cy) / ry <= 1.0f;
	}
	bool IsInsideRoundedRect(float px, float py, float cx, float cy, float halfW, float halfH, float cornerR) noexcept
	{
		const float dx = std::fabs(px - cx);
		const float dy = std::fabs(py - cy);
		const float innerX = halfW - cornerR;
		const float innerY = halfH - cornerR;
		if (dx <= innerX || dy <= innerY)
			return true;
		const float cornerDx = dx - innerX;
		const float cornerDy = dy - innerY;
		return cornerDx * cornerDx + cornerDy * cornerDy <= cornerR * cornerR;
	}
}

Canvas::Canvas(unsigned width, unsigned height, Form form)
	:
	surface(width, height),
	form_(form),
	gpuDirty(true)
{
	ApplyForm(form_);	
}

void Canvas::ApplyForm(Form form) noexcept
{
	form_ = form;
	const unsigned w = surface.GetWidth();
	const unsigned h = surface.GetHeight();
	surface.Clear(Colors::None);
	if (w == 0u || h == 0u)
	{
		gpuDirty = true;
		MarkDirtyAll();
		return;
	}
	if (form == Form::Empty)
	{
		gpuDirty = true;
		MarkDirtyAll();
		return;
	}
	if (form == Form::Rectangle)
	{
		surface.Clear(Colors::White);
		gpuDirty = true;
		MarkDirtyAll();
		return;
	}
	const float cx = (static_cast<float>(w) - 1.0f) * 0.5f;
	const float cy = (static_cast<float>(h) - 1.0f) * 0.5f;
	const float halfW = static_cast<float>(w) * 0.5f;
	const float halfH = static_cast<float>(h) * 0.5f;
	const float rxEllipse = halfW;
	const float ryEllipse = halfH;
	const float rCircle = 0.5f * static_cast<float>(std::min(w, h));
	const float cornerR = 0.25f * static_cast<float>(std::min(w, h));
	for (unsigned y = 0u; y < h; ++y)
	{
		for (unsigned x = 0u; x < w; ++x)
		{
			const float px = static_cast<float>(x) + 0.5f;
			const float py = static_cast<float>(y) + 0.5f;
			bool inside = false;
			switch (form)
			{
			case Form::Ellipse:
				inside = IsInsideEllipse(px, py, cx, cy, rxEllipse, ryEllipse);
				break;
			case Form::Triangle:
				inside = IsInsideTriangleApexUp(px, py, w, h);
				break;
			case Form::Circle:
				inside = IsInsideCircle(px, py, cx, cy, rCircle);
				break;
			case Form::Diamond:
				inside = IsInsideDiamond(px, py, cx, cy, halfW, halfH);
				break;
			case Form::RoundedRectangle:
				inside = IsInsideRoundedRect(px, py, cx, cy, halfW, halfH, cornerR);
				break;
			default:
				break;
			}
			if (inside)
				surface.PutPixel(x, y, Colors::White);
		}
	}
	gpuDirty = true;
	MarkDirtyAll();
}

void Canvas::ReapplyForm() noexcept
{
	ApplyForm(form_);
}

dx::XMMATRIX Canvas::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}

void Canvas::MarkDirtyPixel(unsigned x, unsigned y) noexcept
{
	if (!hasDirtyRect)
	{
		dirtyMinX = dirtyMaxX = x;
		dirtyMinY = dirtyMaxY = y;
		hasDirtyRect = true;
		return;
	}
	dirtyMinX = std::min(dirtyMinX, x);
	dirtyMinY = std::min(dirtyMinY, y);
	dirtyMaxX = std::max(dirtyMaxX, x);
	dirtyMaxY = std::max(dirtyMaxY, y);
}

void Canvas::MarkDirtyAll() noexcept
{
	const unsigned w = surface.GetWidth();
	const unsigned h = surface.GetHeight();
	if (w == 0u || h == 0u)
	{
		hasDirtyRect = false;
		return;
	}
	hasDirtyRect = true;
	dirtyMinX = 0u;
	dirtyMinY = 0u;
	dirtyMaxX = w - 1u;
	dirtyMaxY = h - 1u;
}

void Canvas::NotifyPixelsChanged() noexcept
{
	gpuDirty = true;
	MarkDirtyAll();
}

void Canvas::PutPixel(unsigned x, unsigned y, Color c) noxnd
{
	if (x<0 || x>=surface.GetWidth() || y<0 || y>=surface.GetHeight())
	{
		return;
	}

	surface.PutPixel(x, y, c);
	gpuDirty = true;
	MarkDirtyPixel(x, y);
}

Color Canvas::GetPixel(unsigned x, unsigned y) const noxnd
{
	return surface.GetPixel(x, y);
}

void Canvas::Clear(Color fill) noexcept
{
	surface.Clear(fill);
	gpuDirty = true;
	MarkDirtyAll();
}

void Canvas::Resize(unsigned width, unsigned height)
{
	surface = Surface(width, height);
	surface.Clear(Colors::None);
	gpuDirty = true;
	MarkDirtyAll();
}

unsigned Canvas::GetCanvasWidth() const noexcept
{
	return surface.GetWidth();
}

unsigned Canvas::GetCanvasHeight() const noexcept
{
	return surface.GetHeight();
}

Surface& Canvas::GetSurface() noexcept
{
	return surface;
}

const Surface& Canvas::GetSurface() const noexcept
{
	return surface;
}

void Canvas::ClearGpuDirty() noexcept
{
	gpuDirty = false;
	hasDirtyRect = false;
}