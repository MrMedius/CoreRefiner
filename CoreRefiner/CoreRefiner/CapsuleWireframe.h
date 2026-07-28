#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "ConstantBuffers.h"
#include <DirectXMath.h>

/**
 * @brief Debug capsule gizmo (Unity-like): end rings, hemisphere arcs, cylinder rails.
 * @note Unit mesh: radius 0.5, segment along Y length 1. DoSubmit builds world from A/B/radius.
 */
class CapsuleWireframe : public Drawable
{
public:
	CapsuleWireframe(Graphics& gfx, DirectX::XMFLOAT3 color, std::string tag = "");
	/** @brief Submit from hemisphere centers and radius (world space). */
	void DoSubmit(DirectX::XMFLOAT3 pointA, DirectX::XMFLOAT3 pointB, float radius);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	struct PSColorConstant
	{
		DirectX::XMFLOAT3 color;
		float padding;
	};
	DirectX::XMFLOAT3 mid_{};
	DirectX::XMFLOAT3 axisX_{ 1,0,0 };
	DirectX::XMFLOAT3 axisY_{ 0,1,0 };
	DirectX::XMFLOAT3 axisZ_{ 0,0,1 };
	float scaleXZ_{ 1.0f };
	float scaleY_{ 1.0f };
};
