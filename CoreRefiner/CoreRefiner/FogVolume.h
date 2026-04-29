#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "IndexBuffer.h"
#include "Transformation.h"

class FogVolume : public Drawable
{
public:
	FogVolume(Graphics& gfx, DirectX::XMFLOAT3 size);
	void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	void SetScale(DirectX::XMFLOAT3 pos) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	Transformation trans;
	static constexpr int kLayerCount = 100;
};