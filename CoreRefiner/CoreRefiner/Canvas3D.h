#pragma once
#include "Canvas.h"
#include "Graphics.h"

class Canvas3D : public Canvas
{
public:
	Canvas3D(Graphics& gfx, unsigned width, unsigned height);
	~Canvas3D() override = default;
};