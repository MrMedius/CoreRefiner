#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "IndexBuffer.h"
#include "Transformation.h"

class Player_Head : public Drawable
{
public:
	Player_Head(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
};

class Player_Body : public Drawable
{
public:
	Player_Body(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
};