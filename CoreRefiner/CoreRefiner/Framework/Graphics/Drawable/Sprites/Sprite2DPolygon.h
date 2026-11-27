#pragma once
#include "Sprite.h"
#include "RingParamsCbuf.h"

class Sprite2DPolygon : public Sprite
{
public:
    Sprite2DPolygon(Graphics& gfx, std::vector<std::string> paths, int sides = 30);
    ~Sprite2DPolygon() = default;
	// setters
	void SetRingRange(float start = 0.0f, float end = 359.0f) noexcept;
	void SetRingRatio(float ratio) noexcept;
private:
	// 当前环的起止角和进度。默认从 0 度到 359 度、进度 1。
	Bind::RingParamsCbuf::RingData ringParams{ 0.0f,359.0f,1.0f,0.0f };
	// 这一份多边形正在使用的环参数缓冲。构造里接上。
	std::shared_ptr<Bind::RingParamsCbuf> rpcb_;
};