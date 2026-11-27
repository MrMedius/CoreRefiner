#pragma once
#include "Canvas.h"
#include "Graphics.h"

#include <DirectXMath.h>

class Canvas2DRipples : public Canvas
{
public:
	struct Params
	{
		DirectX::XMFLOAT4 bgColor{ 0.0f, 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 ringColor{ 0.0f, 0.8f, 0.0f, 0.5f };
		float period{ 3.0f };
		float thickness{ 0.05f };
		float ringCount{ 3.0f };
		float aspect{ 1.0f };
	};

	Canvas2DRipples(Graphics& gfx, unsigned width, unsigned height);
	~Canvas2DRipples() override = default;

	void SetParams(const Params& params) noexcept { params_ = params; }
	[[nodiscard]] const Params& GetParams() const noexcept { return params_; }

	static Params MakeDefaultParams(unsigned width, unsigned height) noexcept;

	Params& GetParams() noexcept { return params_; }

private:
	class ParamsCbuf;
	Params params_{};

	unsigned width_{ 0u };
	unsigned height_{ 0u };
};