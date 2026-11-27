#pragma once
#include "Canvas.h"
#include "Graphics.h"

class SliderCanvasFill : public Canvas
{
public:
	struct Params
	{
		float fillAmount{ 0.0f };
		float pad0{ 0.0f };
		float pad1{ 0.0f };
		float pad2{ 0.0f };
	};
	SliderCanvasFill(Graphics& gfx, unsigned width, unsigned height);
	~SliderCanvasFill() override = default;

	void SetParams(const Params& params) noexcept { params_ = params; }
	[[nodiscard]] const Params& GetParams() const noexcept { return params_; }
	Params& GetParams() noexcept { return params_; }

private:
	class ParamsCbuf;

	Params params_{};
};
