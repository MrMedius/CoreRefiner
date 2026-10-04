#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "Transformation.h"
#include "ConstantBuffers.h"

// Debug sphere gizmo: three orthogonal great circles (Unity-style).
// Mesh radius 0.5; DoSubmit diameter scales like CubeWireframe size.
class SphereWireframe : public Drawable
{
public:
	SphereWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag = "");
	void DoSubmit(DirectX::XMFLOAT3 pos, float diameter);
	void DoSubmit(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot, float diameter);
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
