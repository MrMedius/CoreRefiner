#pragma once
#include "Sprite.h"

class Sprite3DNoLit : public Sprite
{
public:
	Sprite3DNoLit(Graphics& gfx, std::vector<std::string> paths);
	~Sprite3DNoLit() = default;
private:
};

