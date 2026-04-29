#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "IndexBuffer.h"
#include "Transformation.h"

class CubeLock : public Drawable
{
public:
	CubeLock(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	void SetScale(DirectX::XMFLOAT3 pos) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	void SpawnControlWindow(Graphics& gfx, const char* name) noexcept;
private:
	Transformation trans;
};