#pragma once
#include "Sprite.h"

class Sprite3D : public Sprite
{
public:
	Sprite3D(Graphics& gfx, std::vector<std::string> paths);
	~Sprite3D() = default;
};