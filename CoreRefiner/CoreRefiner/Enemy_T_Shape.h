#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "IndexBuffer.h"

class Enemy_T_Shape : public Drawable
{
public:
	Enemy_T_Shape(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	// 写出构造时写进像素常量的颜色。
	bool GetMaterialColor(DirectX::XMFLOAT4& out) const noexcept override;
};