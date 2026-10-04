#pragma once
#include "Canvas.h"
#include "Provides.h"
#include <DirectXMath.h>

class Canvas2DSpriteUV : public Canvas, public IProvides<SpriteUVTag>
{
public:
	Canvas2DSpriteUV(Graphics& gfx, unsigned width, unsigned height);
	~Canvas2DSpriteUV() override = default;

	void SetUVOffset(DirectX::XMFLOAT2 offset) noexcept { uvOffset_ = offset; }
	void SetUVScale(DirectX::XMFLOAT2 scale) noexcept { uvScale_ = scale; }
	void SetSampleScale(DirectX::XMFLOAT2 sampleScale) noexcept { sampleScale_ = sampleScale; }
	void SetUVOffset(float x, float y) noexcept { SetUVOffset(DirectX::XMFLOAT2{ x, y }); }
	void SetUVScale(float x, float y) noexcept { SetUVScale(DirectX::XMFLOAT2{ x, y }); }
	void SetSampleScale(float x, float y) noexcept { SetSampleScale(DirectX::XMFLOAT2{ x, y }); }

	[[nodiscard]] DirectX::XMFLOAT2 GetUVOffset() const noexcept { return uvOffset_; }
	[[nodiscard]] DirectX::XMFLOAT2 GetUVScale() const noexcept { return uvScale_; }
	[[nodiscard]] DirectX::XMFLOAT2 GetSampleScale() const noexcept { return sampleScale_; }

	SpriteUVTag::value_type Provide(SpriteUVTag) const noexcept override
	{
		return { uvOffset_, uvScale_, sampleScale_, {} };
	}

private:
	DirectX::XMFLOAT2 uvOffset_{ 0.0f, 0.0f };
	DirectX::XMFLOAT2 uvScale_{ 1.0f, 1.0f };
	DirectX::XMFLOAT2 sampleScale_{ 1.0f, 1.0f };
};
