#include "Canvas.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
	constexpr unsigned kMaxPolygonSides = 64u;
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kDefaultCornerFrac = 0.25f;

	unsigned ResolvePolygonSides(float formParam) noexcept
	{
		const int sides = static_cast<int>(formParam + 0.5f);
		return static_cast<unsigned>(std::clamp(sides, 3, static_cast<int>(kMaxPolygonSides)));
	}
	float ResolveCornerRadiusFraction(float formParam) noexcept
	{
		if (formParam <= 0.0f)
			return kDefaultCornerFrac;
		float frac = formParam;
		if (frac > 1.0f)
			frac *= 0.01f;
		return std::clamp(frac, 0.0f, 0.5f);
	}
	bool IsInsideEllipse(float px, float py, float cx, float cy, float rx, float ry) noexcept
	{
		const float dx = (px - cx) / rx;
		const float dy = (py - cy) / ry;
		return dx * dx + dy * dy <= 1.0f;
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

	bool IsInsideConvexPolygon(float px, float py, const std::array<float, kMaxPolygonSides>& vx,
		const std::array<float, kMaxPolygonSides>& vy, unsigned sides) noexcept
	{
		if (sides < 3u)
			return false;
		bool hasPos = false;
		bool hasNeg = false;
		for (unsigned i = 0u; i < sides; ++i)
		{
			const unsigned j = (i + 1u) % sides;
			const float cross = (vx[j] - vx[i]) * (py - vy[i]) - (vy[j] - vy[i]) * (px - vx[i]);
			if (cross > 0.0f)
				hasPos = true;
			if (cross < 0.0f)
				hasNeg = true;
			if (hasPos && hasNeg)
				return false;
		}
		return true;
	}

	bool IsInsideRegularPolygon(float px, float py, float cx, float cy, float halfW, float halfH, unsigned sides) noexcept
	{
		std::array<float, kMaxPolygonSides> vx{};
		std::array<float, kMaxPolygonSides> vy{};
		const float step = 2.0f * kPi / static_cast<float>(sides);
		const float start = -0.5f * kPi;
		for (unsigned i = 0u; i < sides; ++i)
		{
			const float a = start + step * static_cast<float>(i);
			vx[i] = cx + halfW * std::cos(a);
			vy[i] = cy + halfH * std::sin(a);
		}
		return IsInsideConvexPolygon(px, py, vx, vy, sides);
	}

	// 等腰三角形：顶点在顶边中点，底边为底边两角。
	bool IsInsideIsoscelesTriangle(
		const float px,
		const float py,
		const unsigned w,
		const unsigned h) noexcept
	{
		if (w == 0u || h == 0u)
			return false;

		const float topX = static_cast<float>(w) * 0.5f;
		const float topY = 0.5f;
		const float bottomLeftX = 0.5f;
		const float bottomLeftY = static_cast<float>(h) - 0.5f;
		const float bottomRightX = static_cast<float>(w) - 0.5f;
		const float bottomRightY = static_cast<float>(h) - 0.5f;

		std::array<float, kMaxPolygonSides> vx{};
		std::array<float, kMaxPolygonSides> vy{};
		vx[0] = topX;
		vy[0] = topY;
		vx[1] = bottomLeftX;
		vy[1] = bottomLeftY;
		vx[2] = bottomRightX;
		vy[2] = bottomRightY;
		return IsInsideConvexPolygon(px, py, vx, vy, 3u);
	}
}

Canvas::Canvas(unsigned width, unsigned height, Form form, float formParam)
	:
	surface(width, height),
	form_(form),
	formParam_(formParam),
	gpuDirty(true)
{
	ApplyForm(form_, formParam_);
}

void Canvas::ApplyForm(Form form, float formParam) noexcept
{
	form_ = form;
	if (formParam >= 0.0f)
		formParam_ = formParam;
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
	const unsigned polygonSides = ResolvePolygonSides(formParam_);
	const float cornerFrac = ResolveCornerRadiusFraction(formParam_);
	const float cornerR = cornerFrac * static_cast<float>(std::min(w, h));
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
			case Form::Polygon:
				inside = IsInsideRegularPolygon(px, py, cx, cy, halfW, halfH, polygonSides);
				break;
			case Form::Triangle:
				inside = IsInsideIsoscelesTriangle(px, py, w, h);
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