#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "IndexBuffer.h"
#include "Transformation.h"

class Ball_Shape : public Drawable
{
public:
	Ball_Shape(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
};