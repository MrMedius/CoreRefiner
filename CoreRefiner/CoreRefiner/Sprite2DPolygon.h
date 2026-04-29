#pragma once
#include "Sprite.h"
#include "Provides.h"

class Sprite2DPolygon : public Sprite, public IProvides<RingParamsTag>
{
public:
    Sprite2DPolygon(Graphics& gfx, std::vector<std::string> paths, int sides = 30);
    ~Sprite2DPolygon() = default;
	// setters
	void SetRingRange(float start = 0.0f, float end = 359.0f) noexcept;
	void SetRingRatio(float ratio) noexcept;
	// getters
    RingParamsTag::value_type Provide(RingParamsTag) const noexcept override { return ringParams; }
private:
	RingParamsTag::value_type ringParams{ 0.0f,359.0f,1.0f,0.0f };
};