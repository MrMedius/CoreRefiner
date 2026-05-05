#include "Canvas.h"
#include <algorithm>

Canvas::Canvas(unsigned width, unsigned height)
	:
	surface(width, height),
	gpuDirty(true)
{
	surface.Clear(Colors::None);
	MarkDirtyAll();
}

dx::XMMATRIX Canvas::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}

SpriteUVTag::value_type Canvas::Provide(SpriteUVTag) const noexcept
{
	return { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
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