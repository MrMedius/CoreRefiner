#pragma once
#include "Canvas.h"
#include "Graphics.h"

class Canvas2D : public Canvas
{
public:
	Canvas2D(Graphics& gfx, unsigned width, unsigned height);
	~Canvas2D() override = default;
};