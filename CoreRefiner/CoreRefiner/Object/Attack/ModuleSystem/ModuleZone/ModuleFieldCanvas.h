#pragma once

#include "Canvas.h"
#include "Colors.h"
#include "Graphics.h"

#include <DirectXMath.h>


class ModuleFieldCanvas : public Canvas
{
public:
	static constexpr unsigned kMaxRings = 8u;
	static constexpr float kDefaultFieldSide = 300.0f;

	struct Params
	{
		DirectX::XMFLOAT4 ringColor{ 1.0f, 1.0f, 0.31f, 1.0f };
		float thickness{ 0.01f };
		int ringCount{ 0 };
		float aspect{ 1.0f };
		float pad0{ 0.0f };
		DirectX::XMFLOAT4 rings[kMaxRings]{};
	};

	ModuleFieldCanvas(Graphics& gfx, unsigned width, unsigned height, Color bgColor = Color(100u, 150u, 50u, 150u));
	~ModuleFieldCanvas() override = default;

	void ClearWaves() noexcept;

	void SetWavesLocal(
		const DirectX::XMFLOAT2* centers,
		const float* radii,
		unsigned count,
		float fieldSide = kDefaultFieldSide) noexcept;

	void SetRingColor(DirectX::XMFLOAT4 color) noexcept { params_.ringColor = color; }
	void SetThickness(float uvThickness) noexcept { params_.thickness = uvThickness; }

	[[nodiscard]] Params& GetParams() noexcept { return params_; }
	[[nodiscard]] const Params& GetParams() const noexcept { return params_; }

private:
	class ParamsCbuf;

	Params params_{};
	unsigned width_{ 0u };
	unsigned height_{ 0u };
};

static_assert(sizeof(ModuleFieldCanvas::Params) == 160, "ModuleFieldCanvas::Params must match ScanWaveField_PS cbuffer");