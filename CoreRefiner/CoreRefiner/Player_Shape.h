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
	void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	void SetScale(DirectX::XMFLOAT3 pos) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	Transformation trans;
};

class Player_Body : public Drawable
{
public:
	Player_Body(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	void SetScale(DirectX::XMFLOAT3 pos) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	Transformation trans;
};