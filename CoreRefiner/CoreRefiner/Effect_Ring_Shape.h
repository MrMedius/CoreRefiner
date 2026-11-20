#pragma once
#include "Drawable.h"

#include <DirectXMath.h>

// 半透明圆环。着色器只给它用，参数缓冲也留在这里。
class Effect_Ring_Shape : public Drawable
{
public:
	// 和像素着色器 b2 的布局一致，共 32 字节。
	// color.rgb 是基色，color.a 是整只环的透明度。
	// gradient 外浓内淡开关。innerWave 内圈波浪开关。outerWave 外圈波浪开关。
	// amplitude 波浪幅度系数。开关用 0 和 1。
	struct RingData
	{
		DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float gradient{ 0.0f };
		float innerWave{ 0.0f };
		float outerWave{ 0.0f };
		float amplitude{ 0.0f };
	};

	explicit Effect_Ring_Shape(Graphics& gfx);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;

	// rgb 是基色，不改透明度。
	void SetColor(DirectX::XMFLOAT3 rgb) noexcept;
	// opacity 是整只环的透明度。
	void SetOpacity(float opacity) noexcept;
	// on 为真时外实内虚。
	void SetGradient(bool on) noexcept;
	// on 为真时内沿起浪。
	void SetInnerWave(bool on) noexcept;
	// on 为真时外沿起浪。
	void SetOuterWave(bool on) noexcept;
	// amplitude 是波浪幅度系数。
	void SetAmplitude(float amplitude) noexcept;

private:
	// 绘制前把 data_ 写进 b2。
	class RingCbuf;
	// 这一只环当前的颜色、开关和幅度。
	RingData data_{};
};
