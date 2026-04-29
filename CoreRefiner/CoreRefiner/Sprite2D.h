#pragma once
#include "Sprite.h"

class Sprite2D : public Sprite
{
public:
    Sprite2D(Graphics& gfx, std::vector<std::string> paths);
    ~Sprite2D() = default;
};