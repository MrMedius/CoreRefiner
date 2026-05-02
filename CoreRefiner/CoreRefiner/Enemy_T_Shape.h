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
};