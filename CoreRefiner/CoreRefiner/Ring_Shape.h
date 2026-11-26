#pragma once
#include "Drawable.h"

#include <DirectXMath.h>

// 半透明圆环。着色器只给它用，参数缓冲也留在这里。
class Ring_Shape : public Drawable
{
public:
	// 和像素着色器 b2 的布局一致，共 48 字节。
	// innerColor / outerColor 的 rgb 是这一沿的颜色，a 是这一沿自己的透明度。a 为 0 表示这一沿是空的。
	// opacity 是整只环的播放透明度。淡出只改它。
	// innerWave 内圈波浪开关。outerWave 外圈波浪开关。amplitude 波浪幅度系数。开关用 0 和 1。
	struct RingData
	{
		DirectX::XMFLOAT4 innerColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 outerColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		float opacity{ 1.0f };
		float innerWave{ 0.0f };
		float outerWave{ 0.0f };
		float amplitude{ 0.0f };
	};

	explicit Ring_Shape(Graphics& gfx);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;

	// rgba 是内沿颜色。a 为 0 时内沿是空的，颜色改用外沿。
	void SetInnerColor(DirectX::XMFLOAT4 rgba) noexcept;
	// rgba 是外沿颜色。a 为 0 时外沿是空的，颜色改用内沿。
	void SetOuterColor(DirectX::XMFLOAT4 rgba) noexcept;
	// opacity 是整只环的播放透明度，不改两边渐变色自己的 a。
	void SetOpacity(float opacity) noexcept;
	// on 为真时内沿起浪。
	void SetInnerWave(bool on) noexcept;
	// on 为真时外沿起浪。
	void SetOuterWave(bool on) noexcept;
	// amplitude 是波浪幅度系数。
	void SetAmplitude(float amplitude) noexcept;

private:
	// 绘制前把 data_ 写进 b2。
	class RingCbuf;
	// 这一只环当前的两沿颜色、播放透明度和波浪。
	RingData data_{};
};
