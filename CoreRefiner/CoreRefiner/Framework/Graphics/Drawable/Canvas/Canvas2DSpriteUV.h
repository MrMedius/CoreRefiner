#pragma once
#include "Canvas.h"
#include <DirectXMath.h>
#include <memory>

namespace Bind
{
	class SpriteUVCbuf;
}

class Canvas2DSpriteUV : public Canvas
{
public:
	Canvas2DSpriteUV(Graphics& gfx, unsigned width, unsigned height);
	~Canvas2DSpriteUV() override = default;

	void SetUVOffset(DirectX::XMFLOAT2 offset) noexcept;
	void SetUVScale(DirectX::XMFLOAT2 scale) noexcept;
	void SetSampleScale(DirectX::XMFLOAT2 sampleScale) noexcept;
	void SetUVOffset(float x, float y) noexcept { SetUVOffset(DirectX::XMFLOAT2{ x, y }); }
	void SetUVScale(float x, float y) noexcept { SetUVScale(DirectX::XMFLOAT2{ x, y }); }
	void SetSampleScale(float x, float y) noexcept { SetSampleScale(DirectX::XMFLOAT2{ x, y }); }

	[[nodiscard]] DirectX::XMFLOAT2 GetUVOffset() const noexcept { return uvOffset_; }
	[[nodiscard]] DirectX::XMFLOAT2 GetUVScale() const noexcept { return uvScale_; }
	[[nodiscard]] DirectX::XMFLOAT2 GetSampleScale() const noexcept { return sampleScale_; }

private:
	// 把当前采样起点、裁切缩放和采样窗口写进 UV 缓冲。缓冲还没接上时跳过。
	void SyncUV_() noexcept;

	// 采样起点。
	DirectX::XMFLOAT2 uvOffset_{ 0.0f, 0.0f };
	// 裁切用的缩放。
	DirectX::XMFLOAT2 uvScale_{ 1.0f, 1.0f };
	// 采样窗口。遮罩不改它时，仍用 uvScale_ 做裁切。
	DirectX::XMFLOAT2 sampleScale_{ 1.0f, 1.0f };
	// 这一份画布正在使用的 UV 缓冲。构造里接上。
	std::shared_ptr<Bind::SpriteUVCbuf> uvcb_;
};
