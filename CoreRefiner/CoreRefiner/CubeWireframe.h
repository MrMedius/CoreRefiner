#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "Transformation.h"
#include "ConstantBuffers.h"

class CubeWireframe : public Drawable
{
public:
	CubeWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag = "");
	void DoSubmit(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 size);
	void DoSubmit(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 size);
	void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	void SetScale(DirectX::XMFLOAT3 size) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	struct PSColorConstant
	{
		DirectX::XMFLOAT3 color;
		float padding;
	};
	Transformation trans;
};